Stellarium Web Engine
=====================

About
-----

Stellarium Web Engine is a JavaScript planetarium renderer using
WebGL that can be embedded into a website.

Features
--------

- Atmosphere simulation.
- Gaia stars database access (more than 1 billion stars).
- HiPS surveys rendering.
- High resolution planet textures.
- Constellations.
- Support for adding layers and shapes in the sky view.
- Landscapes.


Build the javascript version
----------------------------

You need to make sure you have both emscripten and sconstruct installed.

    # Setup emscripten path.
    source $PATH_TO_EMSDK/emsdk_env.sh

    # Build stellarium-web-engine.js and stellarium-web-engine.wasm
    # This will also copy the files into html/static/js
    make js

    # Now see apps/simple-html/ to try the library in a browser.


Contributing
------------

In order for your contribution to Stellarium Web Engine to be accepted, you have to sign the
[Stellarium Web Contributor License Agreement (CLA)](doc/cla/sign-cla.md).

## デプロイ方法
cd stellarium-web-engine/apps/web-frontend
yarn install
yarn build
mkdir dist/stellarium
mv dist/* dist/stellarium

aws s3 sync --exact-timestamps --delete dist/ \
s3://bucket-name \
--profile default \
--region ap-northeast-1

aws cloudfront create-invalidation \
--distribution-id DIST-ID \
--paths "/stellarium/*" \
--profile default \
--region ap-northeast-1

