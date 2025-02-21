/* Stellarium Web Engine - Copyright (c) 2022 - Stellarium Labs SRL
 *
 * This program is licensed under the terms of the GNU AGPL v3, or
 * alternatively under a commercial licence.
 *
 * The terms of the AGPL v3 license can be found in the main directory of this
 * repository.
 */

#include "swe.h"
#include <zlib.h> // For crc32.

/*
 * Enum of the label display styles.
 */
enum {
    LABEL_DISPLAY_TRANSLATED    = 0,
    LABEL_DISPLAY_NATIVE        = 1,
};

/*
 * Type: anchor_t
 * An anchor point of a constellation texture.
 */
typedef struct {
    double  uv[2];
    int     hip;
} anchor_t;

/*
 * Type: constellation_t
 * Object representing a single constellation.
 */
typedef struct constellation {
    obj_t       obj;
    constellation_infos_t info;
    fader_t     visible;
    fader_t     image_loaded_fader;

    struct {
        int         nb_stars;
        obj_t       **stars;
        double      (*stars_pos)[3]; // ICRF/observer pos for all stars.
        double      cap[4];  // Bounding cap of the lines (ICRF).
    } lines;

    // Texture and associated transformation matrix.
    struct {
        texture_t   *tex;
        anchor_t    anchors[3];
        obj_t       *anchors_stars[3];
        double      mat[3][3];
        double      cap[4]; // Bounding cap of the image (ICRF)
    } img;

    bool error; // Set if we couldn't parse the stars.

    double last_update; // Last update time in TT

    double pvo[2][4];
    // Radius of the constellation, i.e. the one used when fitting zoom
    double radius;

    // True once all the data for this constellation have been lazily loaded
    // There is no valid pvo, radius, cap etc.. for this object until
    // first_update_complete is true
    bool first_update_complete;
} constellation_t;

/*
 * Type: constellations_t
 * The module object.
 */
typedef struct constellations {
    obj_t       obj;
    fader_t     images_visible;
    fader_t     lines_visible;
    fader_t     bounds_visible;
    fader_t     labels_visible;
    bool        lines_animation;
    bool        show_only_pointed;
    double      illustrations_bscale;
} constellations_t;

static int constellation_update(constellation_t *con, const observer_t *obs);


// Test if a shape in clipping coordinates is clipped or not.
// Support margins.
static bool is_clipped(int n, double (*pos)[4], double mx, double my)
{
    // The six planes equations:
    const double P[6][4] = {
        {-1, 0, 0, -1 + mx}, {1, 0, 0, -1 + mx},
        {0, -1, 0, -1 + my}, {0, 1, 0, -1 + my},
        {0, 0, -1, -1}, {0, 0, 1, -1}
    };
    int i, p;
    for (p = 0; p < 6; p++) {
        for (i = 0; i < n; i++) {
            if (    P[p][0] * pos[i][0] +
                    P[p][1] * pos[i][1] +
                    P[p][2] * pos[i][2] +
                    P[p][3] * pos[i][3] <= 0) {
                break;
            }
        }
        if (i == n) // All the points are outside a clipping plane.
            return true;
    }
    return false;
}

/*
 * Function: join_path
 * Join two file paths.
 */
const char *join_path(const char *base, const char *path, char *buf, int len)
{
    if (!base) {
        snprintf(buf, len, "%s", path);
    } else {
        snprintf(buf, len, "%s/%s", base, path);
    }
    return buf;
}

static void constellation_set_image(obj_t *obj)
{
    constellation_t *cons = (void*)obj;
    char path[1024];
    const constellation_infos_t *a = &cons->info;

    if (cons->img.tex) return; // Already set.
    if (!*a->img) return;

    vec2_copy(a->anchors[0].uv, cons->img.anchors[0].uv);
    vec2_copy(a->anchors[1].uv, cons->img.anchors[1].uv);
    vec2_copy(a->anchors[2].uv, cons->img.anchors[2].uv);
    cons->img.anchors[0].hip = a->anchors[0].hip;
    cons->img.anchors[1].hip = a->anchors[1].hip;
    cons->img.anchors[2].hip = a->anchors[2].hip;

    join_path(cons->info.base_path, cons->info.img, path, sizeof(path));
    cons->img.tex = texture_from_url(path, TF_LAZY_LOAD);
    assert(cons->img.tex);

    cons->image_loaded_fader.target = false;
    cons->image_loaded_fader.value = 0;
}

static int constellation_init(obj_t *obj, json_value *args)
{
    constellation_t *cons = (constellation_t *)obj;
    constellation_infos_t *info;

    fader_init2(&cons->image_loaded_fader, false, 1);
    fader_init2(&cons->visible, false, 0.5);

    // For the moment, since we create the constellation from within C
    // only, we pass the info as a pointer to the structure!
    info = (void*)(intptr_t)json_get_attr_i(args, "info_ptr", 0);
    if (!info) return 0;
    cons->info = *info;
    strcpy(cons->obj.type, "Con");
    constellation_set_image(obj);

    return 0;
}

static int constellation_get_info(const obj_t *obj, const observer_t *obs,
                                  int info, void *out)
{
    const constellation_t *con = (const constellation_t*)obj;
    constellation_update((constellation_t*)con, obs);
    if (!con->first_update_complete) return 1;
    switch (info) {
    case INFO_PVO:
        memcpy(out, con->pvo, sizeof(con->pvo));
        assert(vec3_norm2(con->pvo[0]) > 0);
        return 0;
    case INFO_RADIUS:
        *(double*)out = con->radius;
        assert(con->radius > 0);
        return 0;
    default:
        return 1;
    }
}

// Get the list of the constellation stars.
// Return 0 if all the stars have been loaded (even if we had errors).
static int constellation_create_stars(constellation_t *cons)
{
    int i, nb_err = 0, hip, code;
    assert(cons->lines.stars == NULL);

    // Fetch all the stars used in the constellation lines
    cons->lines.nb_stars = cons->info.nb_lines * 2;
    cons->lines.stars = calloc(cons->info.nb_lines * 2, sizeof(obj_t*));
    for (i = 0; i < cons->info.nb_lines * 2; i++) {
        hip = cons->info.lines[i / 2].hip[i % 2];
        assert(hip);
        cons->lines.stars[i] = obj_get_by_hip(hip, &code);
        if (code == 0) goto still_loading;
        if (!cons->lines.stars[i]) {
            LOG_W("Cannot find line star HIP %d, code=%d", hip, code);
            nb_err++;
        }
    }
    cons->lines.stars_pos = calloc(cons->lines.nb_stars,
                                   sizeof(*cons->lines.stars_pos));

    // Also fetch the 3 stars for the image anchors if there is an illustration
    if (!cons->img.tex) return 0;

    for (i = 0; i < 3; i++) {
        hip = cons->img.anchors[i].hip;
        assert(cons->img.anchors_stars[i] == NULL);
        cons->img.anchors_stars[i] = obj_get_by_hip(hip, &code);
        if (code == 0) goto still_loading;
        if (!cons->img.anchors_stars[i]) {
            LOG_W("Cannot find anchor star HIP %d, code=%d", hip, code);
            nb_err++;
        }
    }

    if (nb_err) {
        LOG_W("%d stars not found in constellation %s",
              nb_err, cons->info.id);
    }

    return 0;

still_loading:
    // Release everything.
    for (i = 0; i < cons->info.nb_lines * 2; i++)
        obj_release(cons->lines.stars[i]);
    free(cons->lines.stars);
    cons->lines.stars = NULL;
    cons->lines.nb_stars = 0;
    for (i = 0; i < 3; i++) {
        obj_release(cons->img.anchors_stars[i]);
        cons->img.anchors_stars[i] = NULL;
    }

    return 1;
}

// Extends a cap to include a given point, without changing the cap direction.
static bool cap_extends(double cap[4], double p[static 3])
{
    double n[3];
    const int old_acos = cap[3];
    vec3_normalize(p, n);
    cap[3] = fmin(cap[3], vec3_dot(cap, n));
    return old_acos != cap[3];
}

// Compute the cap of an image from its 3d mat.
static void compute_image_cap(const double mat[3][3], double cap[4])
{
    int i;
    double p[4];

    // Center point (UV = [0.5, 0.5])
    mat3_mul_vec3(mat, VEC(0.5, 0.5, 1.0), p);
    vec3_normalize(p, cap);
    cap[3] = 1.0;
    for (i = 0; i < 4; i++) {
        mat3_mul_vec3(mat, VEC(i / 2, i % 2, 1.0), p);
        vec3_normalize(p, p);
        cap_extends(cap, p);
    }
}

static void update_image_mat(constellation_t *cons, const observer_t *obs)
{
    int i, r;
    double pos[3][3];
    double uvs[3][3];
    double tmp[3][3];
    double pvo[2][4];
    obj_t *star;

    if (!cons->img.tex) return;
    for (i = 0; i < 3; i++) {
        vec2_copy(cons->img.anchors[i].uv, uvs[i]);
        uvs[i][2] = 1.0;
        star = cons->img.anchors_stars[i];
        assert(star);
        if (!star) goto error;
        obj_get_pvo(star, obs, pvo);
        vec3_normalize(pvo[0], pos[i]);
    }
    // Compute the transformation matrix M from uv to ICRS:
    // M . uv = pos  =>   M = pos * inv(uv)
    r = mat3_invert(uvs, tmp);
    if (!r) goto error;
    mat3_mul(pos, tmp, cons->img.mat);
    compute_image_cap(cons->img.mat, cons->img.cap);
    assert(cons->img.mat[2][2]);
    return;

error:
    LOG_W("Cannot compute image for constellation %s", cons->info.id);
    texture_release(cons->img.tex);
    cons->img.tex = NULL;
}

// Make a line shorter so that we don't hide the star.
static void line_truncate(double pos[2][4], double a0, double a1)
{
    double rv[3], rv2[3];       // Rotation vector.
    double u;
    double rvm[3][3];   // Rotation matrix.
    eraPxp(pos[0], pos[1], rv);
    eraPn(rv, &u, rv);

    eraSxp(-a0, rv, rv2);
    eraRv2m(rv2, rvm);
    eraRxp(rvm, pos[0], pos[0]);

    eraSxp(+a1, rv, rv2);
    eraRv2m(rv2, rvm);
    eraRxp(rvm, pos[1], pos[1]);
}

/*
 * Function: line_animation_effect
 *
 * Apply an animation effect to a line.  The effect here is simply to grow
 * the line from one point to the other
 *
 * Parameters:
 *   pos    - The line two points positions.
 *   k      - The effect time from 0 (start) to 1 (end).
 */
static void line_animation_effect(double pos[2][4], double k)
{
    double p[3];
    k = smoothstep(0.0, 1.0, k); // Smooth transition speed.
    vec3_mix(pos[0], pos[1], k, p);
    vec3_normalize(p, pos[1]);
}

static void constellation_compute_best_cap(constellation_t *con,
                                          const observer_t *obs) {
    int outermost_star = -1, opposite_star = -1;
    double v[3], vv[3];
    double cap2[4];
    double max_d = -1;
    int i;

    // Compute bounding cap, and the outermost star
    vec3_copy(con->pvo[0], con->lines.cap);
    con->lines.cap[3] = 1.0;
    for (i = 0; i < con->lines.nb_stars; i++) {
        if (!con->lines.stars[i]) continue;
        if (cap_extends(con->lines.cap, con->lines.stars_pos[i])) {
            outermost_star = i;
        }
    }
    // Use the outermost and the star at the extrem opposit to attempt
    // to find a better cap (cap2)
    if (outermost_star == -1) return;
    vec3_sub(con->lines.cap, con->lines.stars_pos[outermost_star], v);
    vec3_normalize(v, v);
    for (i = 0; i < con->lines.nb_stars; i++) {
        if (!con->lines.stars[i]) continue;
        vec3_sub(con->lines.stars_pos[i], con->lines.cap, vv);
        const double d = vec3_dot(v, vv);
        if (d > max_d) {
            opposite_star = i;
            max_d = d;
        }
    }
    assert(opposite_star != -1);
    vec3_copy(con->lines.stars_pos[outermost_star], v);
    vec3_add(con->lines.stars_pos[opposite_star], v, v);
    vec3_normalize(v, cap2);
    cap2[3] = 1.0;
    for (i = 0; i < con->lines.nb_stars; i++) {
        if (!con->lines.stars[i]) continue;
        cap_extends(cap2, con->lines.stars_pos[i]);
    }
    // If cap2 is better than the base cap, use it instead
    if (cap2[3] > con->lines.cap[3])
        vec4_copy(cap2, con->lines.cap);
    assert(vec3_norm2(con->lines.cap) > 0);
}

/*
 * Load the stars and the image matrix.
 * Return 0 if we are ready to render.
 */
static int constellation_update(constellation_t *con, const observer_t *obs)
{
    // The position of a constellation is its middle point.
    double pvo[2][4], pos[4] = {0, 0, 0, 0};
    int i;
    if (con->error) return -1;

    if (con->first_update_complete &&
            fabs(obs->tt - con->last_update) < 365.0) {
        // Constellation shape change cannot be seen over the course of
        // one year
        return 0;
    }
    if (!con->first_update_complete) {
        if (constellation_create_stars(con)) {
            return 1;
        }
    }

    if (con->lines.nb_stars > 0) {
        for (i = 0; i < con->lines.nb_stars; i++) {
            if (!con->lines.stars[i]) continue;
            obj_get_pvo(con->lines.stars[i], obs, pvo);
            vec3_normalize(pvo[0], con->lines.stars_pos[i]);
            vec3_add(pos, con->lines.stars_pos[i], pos);
        }
        if (!vec3_norm2(pos)) {
            // We couldn't load any star, so we won't be able to determine
            // a proper direction, just give up with this constellation..
            LOG_E("Could not load constellation %s.", con->info.id);
            con->error = true;
            return -1;
        }
        vec3_normalize(pos, pos);
        vec3_copy(pos, con->pvo[0]);

        constellation_compute_best_cap(con, obs);

        if (con->lines.cap[3] >= 1) {
            // Lines for this constellation have no proper segments, it's
            // probably a single star constellation, in this case we use a
            // default cap radius of 1 deg
            con->lines.cap[3] = cos(1. * DD2R);
        }
    }

    // If the constellation has no lines, it must have an illustration
    if (con->lines.nb_stars == 0 && !con->img.tex) {
        con->error = true;
        LOG_E("Invalid constellation %s has no lines and no illustration",
              con->info.id);
        return -1;
    }

    update_image_mat(con, obs);

    if (con->lines.nb_stars == 0) {
        // If the constellation has no lines, use the image cap as a
        // substitute for the lines cap
        vec4_copy(con->img.cap, con->lines.cap);
        assert(vec3_norm2(con->lines.cap) > 0);
    }

    // Use the new cap center as constellation position
    vec3_copy(con->lines.cap, con->pvo[0]);
    con->pvo[0][3] = 0; // At infinity.
    vec4_set(con->pvo[1], 0, 0, 0, 0);
    con->radius = acos(con->lines.cap[3]);
    assert(con->radius > 0);

    con->last_update = obs->tt;
    con->first_update_complete = true;
    assert(vec3_norm2(con->lines.cap) > 0);
    return 0;
}

static void spherical_project(
        const uv_map_t *map, const double v[2], double out[4])
{
    // Rotation matrix from 1875.0 to J2000.  Computed with erfa:
    //     eraEpb2jd(1875.0, &djm0, &djm);
    //     eraPnm06a(djm0, djm, rnpb);
    const double rnpb[3][3] = {
        {0.999535020565168, 0.027962538774844, 0.012158909862936},
        {-0.027962067406873, 0.999608963139696, -0.000208799220464},
        {-0.012159993837296, -0.000131286124061, 0.999926055923052},
    };

    vec3_from_sphe(v[0], v[1], out);
    out[3] = 0.0; // At infinity.

    // Since the edges coordinates are in B1875.0, they are not exaclty
    // aligned with the meridians and parallels we need to apply the
    // rotation to J2000.
    mat3_mul_vec3(rnpb, out, out);
}

static int render_bounds(const constellation_t *con,
                         const painter_t *painter_,
                         bool selected)
{
    int i;
    const constellation_infos_t *info;
    double line[2][4] = {};
    painter_t painter = *painter_;
    const constellations_t *cons = (const constellations_t*)con->obj.parent;
    uv_map_t map = {
        .map = spherical_project,
    };

    assert(con->first_update_complete);

    if (!selected) {
        painter.color[3] *= cons->bounds_visible.value * con->visible.value;
    }
    if (!painter.color[3]) return 0;
    assert(vec3_norm2(con->lines.cap) > 0);
    if (painter_is_cap_clipped(&painter, FRAME_ICRF, con->lines.cap))
        return 0;

    if (selected)
        vec4_set(painter.color, 1.0, 0.84, 0.84, 0.5 * painter.color[3]);
    else
        vec4_set(painter.color, 0.8, 0.34, 0.34, 0.4 * painter.color[3]);
    painter.lines.dash_ratio = 0.85;
    painter.lines.dash_length = 8;
    info = &con->info;
    if (!info) return 0;
    for (i = 0; i < info->nb_edges; i++) {
        memcpy(line[0], info->edges[i][0], 2 * sizeof(double));
        memcpy(line[1], info->edges[i][1], 2 * sizeof(double));
        if (line[1][0] < line[0][0]) line[1][0] += 2 * M_PI;
        paint_line(&painter, FRAME_ICRF, line, &map, 0,
                   PAINTER_SKIP_DISCONTINUOUS);
    }
    return 0;
}

/*
 * Function: constellation_lines_in_view
 * Check if the constellation is visible.
 *
 * We add a small margin on the side so that we don't render the
 * constellations when they are too close to the side of the screen.
 *
 * We need this test so that the constellation visible fader gets properly
 * updated as soon as the constellation get in and out of the screen.
 */
static bool constellation_lines_in_view(const constellation_t *con,
                                        const painter_t *painter)
{
    int i, nb = 0;
    double (*pos)[4], mx, my;
    bool ret;
    const double m = 100; // Border margins (windows unit).

    assert(con->first_update_complete);

    // First fast tests for the case when the constellation is not in the
    // screen at all.
    assert(vec3_norm2(con->lines.cap) > 0);
    if (painter_is_cap_clipped(painter, FRAME_ICRF, con->lines.cap))
        return false;

    // Clipping test.
    pos = calloc(con->lines.nb_stars, sizeof(*pos));
    for (i = 0; i < con->lines.nb_stars; i++) {
        if (!con->lines.stars[i]) continue;
        convert_frame(painter->obs, FRAME_ICRF, FRAME_VIEW, true,
                      con->lines.stars_pos[i], pos[nb]);
        project_to_clip(painter->proj, pos[nb], pos[nb]);
        nb++;
    }
    if (nb == 0) {
        free(pos);
        return true;
    }
    // Compute margins in NDC.
    mx = m * painter->pixel_scale / painter->fb_size[0] * 2;
    my = m * painter->pixel_scale / painter->fb_size[1] * 2;
    mx = fmin(mx, 0.5);
    my = fmin(my, 0.5);

    ret = !is_clipped(con->lines.nb_stars, pos, mx, my);
    free(pos);
    return ret;
}

/*
 * Function: constellation_image_in_view
 * Check if the constellation image is visible.
 *
 * We add a small margin on the side so that we don't render the
 * constellations when they are too close to the side of the screen.
 */
static bool constellation_image_in_view(const constellation_t *con,
                                        const painter_t *painter)
{
    int i;
    double pos[4][4], mx, my;
    bool ret;
    const double m = 100; // Border margins (windows unit).
    const observer_t *obs = painter->obs;

    assert(con->first_update_complete);

    // Check that the texture matrix is computed.
    if (!con->img.tex) return false;

    // First fast tests for the case when the constellation is not in the
    // screen at all.
    if (painter_is_cap_clipped(painter, FRAME_ICRF, con->img.cap))
        return false;

    // Clipping test.
    for (i = 0; i < 4; i++) {
        vec4_set(pos[i], i / 2, i % 2, 1, 0);
        mat3_mul_vec3(con->img.mat, pos[i], pos[i]);
        vec3_normalize(pos[i], pos[i]);
        convert_frame(obs, FRAME_ICRF, FRAME_VIEW, true, pos[i], pos[i]);
        project_to_clip(painter->proj, pos[i], pos[i]);
    }

    // Compute margins in NDC.
    mx = m * painter->pixel_scale / painter->fb_size[0] * 2;
    my = m * painter->pixel_scale / painter->fb_size[1] * 2;
    mx = fmin(mx, 0.5);
    my = fmin(my, 0.5);

    ret = !is_clipped(4, pos, mx, my);
    return ret;
}

static int render_lines(constellation_t *con, const painter_t *_painter,
                        bool selected)
{
    painter_t painter = *_painter;
    int i;
    double (*lines)[4];
    double lines_color[4];
    double mag[2], radius[2], visible, opacity;
    const observer_t *obs = painter.obs;
    const constellations_t *cons = (const constellations_t*)con->obj.parent;

    assert(con->first_update_complete);

    visible = cons->lines_visible.value * con->visible.value;
    if (selected) {
        visible = 1;
        painter.lines.width *= 2;
    }
    if (painter.color[3] == 0.0 || visible == 0.0) return 0;
    assert(vec3_norm2(con->lines.cap) > 0);
    if (painter_is_cap_clipped(&painter, FRAME_ICRF, con->lines.cap))
        return 0;

    vec4_set(lines_color, 0.65, 1.0, 1.0, 0.4);
    vec4_emul(lines_color, painter.color, painter.color);

    lines = calloc(con->lines.nb_stars, sizeof(*lines));
    for (i = 0; i < con->lines.nb_stars; i++) {
        if (!con->lines.stars[i]) continue;
        vec3_copy(con->lines.stars_pos[i], lines[i]);
        lines[i][3] = 0; // To infinity.
    }

    for (i = 0; i < con->lines.nb_stars; i += 2) {
        if (!con->lines.stars[i + 0] || !con->lines.stars[i + 1]) continue;
        obj_get_info(con->lines.stars[i + 0], obs, INFO_VMAG, &mag[0]);
        obj_get_info(con->lines.stars[i + 1], obs, INFO_VMAG, &mag[1]);
        core_get_point_for_mag(mag[0], &radius[0], NULL);
        core_get_point_for_mag(mag[1], &radius[1], NULL);
        radius[0] = core_get_apparent_angle_for_point(painter.proj, radius[0]);
        radius[1] = core_get_apparent_angle_for_point(painter.proj, radius[1]);
        // Add some space, using ad-hoc formula.
        line_truncate(&lines[i], radius[0] * 1.0 + 0.2 * DD2R,
                                 radius[1] * 1.0 + 0.2 * DD2R);
        if (cons->lines_animation)
            line_animation_effect(&lines[i], visible * 2);
    }

    opacity = painter.color[3];
    for (i = 0; i < con->lines.nb_stars; i += 2) {
        if (!con->lines.stars[i + 0] || !con->lines.stars[i + 1]) continue;
        painter.color[3] = opacity;
        if (con->info.lines[i / 2].line_weight == LINE_WEIGHT_THIN)
            painter.color[3] *= 0.25;
        else if (con->info.lines[i / 2].line_weight == LINE_WEIGHT_BOLD)
            painter.color[3] *= 1.6;
        paint_line(&painter, FRAME_ICRF, lines + i, NULL, 1,
                   PAINTER_SKIP_DISCONTINUOUS);
    }

    free(lines);

    return 0;
}

// Project from uv to the sphere.
static void img_map(const uv_map_t *map, const double v[2], double out[4])
{
    double uv[3] = {v[0], v[1], 1.0};
    mat3_mul_vec3(map->mat, uv, out);
    vec3_normalize(out, out);
    out[3] = 0;
}

static int render_img(constellation_t *con, const painter_t *painter_,
                      bool selected)
{
    uv_map_t map = {0};
    painter_t painter = *painter_;
    const constellations_t *cons = (const constellations_t*)con->obj.parent;

    // Fade out image as we zoom in.
    double diam = acos(con->img.cap[3]) * DR2D * 2;

    assert(con->first_update_complete);

    painter.color[3] *= smoothstep(diam / 5, diam / 2, core->fov * DR2D);
    if (!selected) {
        painter.color[3] *= cons->images_visible.value * con->visible.value;
    }
    painter.color[3] *= cons->illustrations_bscale;
    if (!painter.color[3]) return 0;
    // Skip if not ready yet.
    if (!con->img.tex || !texture_load(con->img.tex, NULL)) return 0;

    if (painter_is_cap_clipped(&painter, FRAME_ICRF, con->img.cap))
        return 0;

    con->image_loaded_fader.target = true;

    painter.flags |= PAINTER_ADD;
    vec3_set(painter.color, 1, 1, 1);
    painter.color[3] *= (selected ? 0.6 : 0.3) * con->image_loaded_fader.value;
    mat3_copy(con->img.mat, map.mat);
    map.map = img_map;
    painter_set_texture(&painter, PAINTER_TEX_COLOR, con->img.tex, NULL);
    paint_quad(&painter, FRAME_ICRF, &map, 4);
    return 0;
}

static bool constellation_is_pointed(const constellation_t *con,
                                     const painter_t *painter)
{
    // Cache the last computation of the center IAU csts.
    static uint64_t hash = 0;
    static char cst[4][8] = {};
    const observer_t *obs = painter->obs;
    const projection_t *proj = painter->proj;
    double p[4];
    int i;
    const double d = 10;

    if (hash != obs->hash) {
        hash = obs->hash;
        for (i = 0; i < 4; i++) {
            vec4_set(p, proj->window_size[0] / 2 + (i % 2 - 0.5) * d,
                        proj->window_size[1] / 2 + (i / 2 - 0.5) * d +
                    core->y_offset, 0, 0);
            unproject(proj, p, p);
            vec3_normalize(p, p);
            convert_frame(obs, FRAME_VIEW, FRAME_ICRF, true, p, p);
            find_constellation_at(p, cst[i]);
        }
    }

    for (i = 0; i < 4; i++) {
        if (strncasecmp(con->info.iau, cst[i], 3) == 0)
            return true;

        // Special quick fix for some constellations that share an image
        // with an other (Puppis, Vela => Carina, and Serpens => Ophiuchus.
        if (strcmp(con->info.iau, "Car") == 0) {
            if (strncasecmp(cst[i], "Pup", 3) == 0) return true;
            if (strncasecmp(cst[i], "Vel", 3) == 0) return true;
        }
        if (strcmp(con->info.iau, "Oph") == 0) {
            if (strncasecmp(cst[i], "Ser", 3) == 0) return true;
        }

    }
    return false;
}


// Extra tests to avoid rendering too many labels.
static bool should_render_label(
        const constellation_t *con, int label_len,
        const painter_t *painter, bool selected)
{
    const constellations_t *cons = (constellations_t*)con->obj.parent;
    double label_pixel_length, pixel_angular_resolution;
    double win_pos[2], p[3], label_cap[4];

    if (selected) return true;

    if (cons->show_only_pointed && constellation_is_pointed(con, painter))
        return true;

    // Don't render the label if its center is not visible.
    if (painter_is_point_clipped_fast(
                painter, FRAME_ICRF, con->lines.cap, true)) {
        return false;
    }

    // Estimate the label bounding cap.  Only render the label if it fits
    // entirely into the line bounding cap.
    // Add an exception for constellation with a single point line: in
    // that case always render the label (see Kamilaroi sky culture).
    if (con->lines.cap[3] == 1) return true;
    painter_project(painter, FRAME_ICRF, con->lines.cap, true, false, win_pos);
    win_pos[0] += 1;
    painter_unproject(painter, FRAME_ICRF, win_pos, p);
    pixel_angular_resolution = acos(vec3_dot(con->lines.cap, p));
    label_pixel_length = 0.5 * FONT_SIZE_BASE * 1.4 * label_len;
    vec3_copy(con->lines.cap, label_cap);
    label_cap[3] = cos(label_pixel_length / 2 * pixel_angular_resolution);

    if (!cap_contains_cap(con->lines.cap, label_cap))
        return false;

    return true;
}

static int render_label(constellation_t *con, const painter_t *painter_,
                        bool selected)
{
    painter_t painter = *painter_;
    double names_color[4];
    char label[256];
    const char* res;
    const char* name;
    constellations_t *cons = (constellations_t*)con->obj.parent;
    int max_label_len;

    assert(con->first_update_complete);
    if (!selected) {
        painter.color[3] *= cons->labels_visible.value * con->visible.value;
    }
    if (painter.color[3] == 0.0) return 0;
    assert(vec3_norm2(con->lines.cap) > 0);
    if (painter_is_cap_clipped(&painter, FRAME_ICRF, con->lines.cap))
        return 0;

    res = skycultures_get_label(con->info.id, label, sizeof(label));
    if (!res) {
        snprintf(label, sizeof(label), "%s", con->info.id);
    }

    name = translate_jp(label);

    max_label_len = u8_split_line(label, sizeof(label), label, 15);

    if (!should_render_label(con, max_label_len, &painter, selected))
        return 0;

    if (!selected)
        vec4_set(names_color, 0.65, 1.0, 1.0, 0.6 * painter.color[3]);
    else
        vec4_set(names_color, 1.0, 1.0, 1.0, 1.0);

    labels_add_3d(name, FRAME_ICRF,
                  con->lines.cap, true, 0, FONT_SIZE_BASE,
                  names_color, 0, ALIGN_CENTER | ALIGN_MIDDLE,
                  TEXT_UPPERCASE | TEXT_SPACED | (selected ? TEXT_BOLD : TEXT_BOLD) |
                  TEXT_MULTILINES,
                  0, &con->obj);

    return 0;
}

static bool constellation_is_visible(const constellation_t *con,
                                     const painter_t *painter)
{
    const constellations_t *cons = (const constellations_t*)con->obj.parent;

    if (!con->first_update_complete) return false;

    // Show selected constellation no matter what.
    if (core->selection && &con->obj == core->selection)
        return true;

    // Only show non centered constellations if 'show_only_pointed' is set.
    if (cons->show_only_pointed && con->info.iau[0]) {
        return constellation_is_pointed(con, painter);
    }

    // Test either the image or the line.
    if (constellation_lines_in_view(con, painter)) return true;
    if (constellation_image_in_view(con, painter)) return true;
    return false;
}


static int constellation_render(obj_t *obj, const painter_t *_painter)
{
    constellation_t *con = (constellation_t*)obj;
    painter_t painter = *_painter;
    const bool selected = core->selection && obj == core->selection;
    const constellations_t *cons = (const constellations_t*)con->obj.parent;
    assert(cons);

    if (constellation_update(con, painter.obs))
        return 0;
    if (con->error)
        return 0;

    assert(con->first_update_complete);

    // Early exit if nothing should be rendered.
    if (    !selected
            && cons->lines_visible.value == 0.0
            && cons->labels_visible.value == 0.0
            && cons->images_visible.value == 0.0
            && cons->bounds_visible.value == 0.0)
        return 0;

    con->visible.target = constellation_is_visible(con, &painter);
    if (con->visible.value == 0.0) return 0;

    render_lines(con, &painter, selected);
    render_label(con, &painter, selected);
    render_img(con, &painter, selected);
    render_bounds(con, &painter, selected);

    return 0;
}

static int constellation_render_pointer(
        const obj_t *obj, const painter_t *painter)
{
    // Do nothing: selected constellations are simply rendered with different
    // colors.
    return 0;
}

static void constellation_del(obj_t *obj)
{
    int i;
    constellation_t *con = (constellation_t*)obj;
    texture_release(con->img.tex);
    for (i = 0; i < con->lines.nb_stars; i++) {
        obj_release(con->lines.stars[i]);
    }
    free(con->lines.stars);
    free(con->lines.stars_pos);
}

static void constellation_get_2d_ellipse(const obj_t *obj,
                               const observer_t *obs,
                               const projection_t* proj,
                               double win_pos[2], double win_size[2],
                               double* win_angle)
{
    const constellation_t *con = (const constellation_t*)obj;
    double ra, de;

    painter_t tmp_painter;
    tmp_painter.obs = obs;
    tmp_painter.proj = proj;
    vec3_to_sphe(con->lines.cap, &ra, &de);
    double size_x = acos(con->lines.cap[3]) * 2;
    painter_project_ellipse(&tmp_painter, FRAME_ICRF, ra, de, 0,
                            size_x, size_x, win_pos, win_size, win_angle);
    win_size[0] /= 2.0;
    win_size[1] /= 2.0;
}

static void constellation_get_designations(
    const obj_t *obj, void *user,
    int (*f)(const obj_t *obj, void *user, const char *cat, const char *str))
{
    const constellation_t *cst = (const constellation_t*)obj;
    f(obj, user, "CON", ((const char *)cst->info.id) + 4);
    if (cst->info.iau[0])
        f(obj, user, "NAME", (const char *)cst->info.iau);
}

static int constellations_init(obj_t *obj, json_value *args)
{
    constellations_t *conss = (void*)obj;
    conss->lines_animation = true;
    conss->show_only_pointed = true;
    conss->illustrations_bscale = 1.0;
    fader_init(&conss->lines_visible, false);
    fader_init(&conss->labels_visible, false);
    fader_init(&conss->images_visible, false);
    fader_init(&conss->bounds_visible, false);
    return 0;
}

static int constellations_update(obj_t *obj, double dt)
{
    constellation_t *con;
    constellations_t *cons = (constellations_t*)obj;

    fader_update(&cons->images_visible, dt);
    fader_update(&cons->lines_visible, dt);
    fader_update(&cons->labels_visible, dt);
    fader_update(&cons->bounds_visible, dt);

    // Skip update if not visible.
    if (cons->lines_visible.value == 0.0 &&
        cons->images_visible.value == 0.0 &&
        cons->bounds_visible.value == 0.0 &&
        cons->labels_visible.value == 0.0 &&
        (!core->selection || core->selection->parent != obj)) return 0;

    MODULE_ITER(obj, con, "constellation") {
        fader_update(&con->image_loaded_fader, dt);
        fader_update(&con->visible, dt);
    }
    return 0;
}

static int constellations_render(obj_t *obj, const painter_t *painter)
{
    constellations_t *cons = (constellations_t*)obj;
    constellation_t *con;
    if (cons->lines_visible.value == 0.0 &&
        cons->labels_visible.value == 0.0 &&
        cons->images_visible.value == 0.0 &&
        cons->bounds_visible.value == 0.0 &&
        (!core->selection || core->selection->parent != obj)) return 0;

    MODULE_ITER(obj, con, "constellation") {
        obj_render((obj_t*)con, painter);
    }
    return 0;
}

static json_value *constellation_get_json_data(const obj_t *obj)
{
    json_value* ret = json_object_new(0);
    const constellation_t *con = (const constellation_t*)obj;
    if (con->info.description) {
        json_object_push(ret, "description",
                         json_string_new(con->info.description));
    }
    return ret;
}

/*
 * Meta class declarations.
 */

static obj_klass_t constellation_klass = {
    .id             = "constellation",
    .size           = sizeof(constellation_t),
    .init           = constellation_init,
    .get_info       = constellation_get_info,
    .render         = constellation_render,
    .render_pointer = constellation_render_pointer,
    .del            = constellation_del,
    .get_json_data  = constellation_get_json_data,
    .get_designations = constellation_get_designations,
    .get_2d_ellipse = constellation_get_2d_ellipse,
    .attributes = (attribute_t[]) {
        PROPERTY(description, TYPE_STRING_PTR,
                 MEMBER(constellation_t, info.description)),
        {}
    }
};
OBJ_REGISTER(constellation_klass)

static obj_klass_t constellations_klass = {
    .id = "constellations",
    .size = sizeof(constellations_t),
    .flags = OBJ_IN_JSON_TREE | OBJ_MODULE | OBJ_LISTABLE,
    .init = constellations_init,
    .update = constellations_update,
    .render = constellations_render,
    .render_order = 25,
    .attributes = (attribute_t[]) {
        PROPERTY(lines_visible, TYPE_BOOL,
                 MEMBER(constellations_t, lines_visible.target)),
        PROPERTY(labels_visible, TYPE_BOOL,
                 MEMBER(constellations_t, labels_visible.target)),
        PROPERTY(images_visible, TYPE_BOOL,
                 MEMBER(constellations_t, images_visible.target)),
        PROPERTY(bounds_visible, TYPE_BOOL,
                 MEMBER(constellations_t, bounds_visible.target)),
        PROPERTY(lines_animation, TYPE_BOOL,
                 MEMBER(constellations_t, lines_animation)),
        PROPERTY(show_only_pointed, TYPE_BOOL,
                 MEMBER(constellations_t, show_only_pointed)),
        PROPERTY(illustrations_bscale, TYPE_FLOAT,
                 MEMBER(constellations_t, illustrations_bscale)),
        {}
    },
};
OBJ_REGISTER(constellations_klass);

