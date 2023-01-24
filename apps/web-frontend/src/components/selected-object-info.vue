// Stellarium Web - Copyright (c) 2022 - Stellarium Labs SRL
//
// This program is licensed under the terms of the GNU AGPL v3, or
// alternatively under a commercial licence.
//
// The terms of the AGPL v3 license can be found in the main directory of this
// repository.

<template>
  <v-card v-if="selectedObject" transparent style="background: rgba(66, 66, 66, 0.3);">
    <v-btn icon style="position: absolute; right: 0" v-on:click.native="unselect()">
      <v-icon>mdi-close</v-icon>
    </v-btn>
    <v-card-title primary-title>
      <div style="width: 100%">
        <img :src="icon" height="48" width="48" align="left" style="margin-top: 3px; margin-right: 10px"/>
        <div style="overflow: hidden; text-overflow: ellipsis;">
          <div class="text-h5">{{ title }}</div>
          <div class="grey--text text-body-2">{{ type }}</div>
        </div>
      </div>
    </v-card-title>
    <v-card-text style="padding-bottom: 5px;"></v-card-text>
    <v-card-text>
      <template v-for="item in items">
        <v-row style="width: 100%" :key="item.key" no-gutters>
          <v-col cols="4" style="color: #dddddd">{{ item.key }}</v-col>
          <v-col cols="8" style="font-weight: 500" class="white--text"><span v-html="item.value"></span></v-col>
        </v-row>
      </template>
    </v-card-text>
    <v-card-actions style="margin-top: -25px">
      <v-spacer/>
      <template v-for="item in pluginsSelectedInfoExtraGuiComponents">
        <component :is="item" :key="item"></component>
      </template>
    </v-card-actions>
  </v-card>
</template>

<script>

import Moment from 'moment'
import swh from '@/assets/sw_helpers.js'

const fullWordTranslation = new Map([
  ['Small Magellanic Cloud', '小マゼラン雲'],
  ['47 Tuc', 'きょしちょう座47']
])

const translation = new Map([
  ['Sum', '太陽'],
  ['Moon', '月'],
  ['Mercury', '水星'],
  ['Venus', '金星'],
  ['Earth', '地球'],
  ['Mars', '火星'],
  ['Jupiter', '木星'],
  ['Saturn', '土星'],
  ['Uranus', '天王星'],
  ['Neptune', '海王星'],
  ['Pluto', '"冥王星'],
  ['Sirius', 'シリウス'],
  ['Canopus', 'カノープス'],
  ['Arcturus', 'アークトゥルス'],
  ['Rigil', 'リギル'],
  ['Kentaurus', 'ケンタウルス'],
  ['Vega', 'ベガ'],
  ['Capella', 'カペラ'],
  ['Rigel', 'リゲル'],
  ['Procyon', 'プロキオン'],
  ['Betelgeuse', 'ベテルギウス'],
  ['Achernar', 'アケルナル'],
  ['Hadar', 'ハダル'],
  ['Acrux', 'アクルックス'],
  ['Altair', 'アルタイル'],
  ['Aldebaran', 'アルデバラン'],
  ['Capella', 'カペラ'],
  ['Antares', 'アンタレス'],
  ['Spica', 'スピカ'],
  ['Pollux', 'ポルックス'],
  ['Fomalhaut', 'フォーマルハウト'],
  ['Deneb', 'デネブ'],
  ['Mimosa', 'ミモザ'],
  ['Toliman', 'トリマン'],
  ['Regulus', 'レグルス'],
  ['Adara', 'アダーラ'],
  ['Castor', 'カストル'],
  ['Shaula', 'シャウラ'],
  ['Gacrux', 'ガクルックス'],
  ['Bellatrix', 'ベラトリックス'],
  ['Elnath', 'エルナト'],
  ['Miaplacidus', 'ミアプラキドゥス'],
  ['Alnilam', 'アルニラム'],
  ['Al Na\'ir', 'アルナイル'],
  ['Al Suhail al-Muhlif', 'スハイル・ムーリフ、レゴール'],
  ['Alioth', 'アリオト'],
  ['Alnitak', 'アルニタク'],
  ['Dubhe', 'ドゥーベ'],
  ['Mirfak', 'ミルファク'],
  ['Wezen', 'ウェズン'],
  ['Kaus', 'カウス'],
  ['Australis', 'アウストラリス'],
  ['Alkaid', 'アルカイド'],
  ['Sargas', 'サルガス'],
  ['Menkalinan', 'メンカリナン'],
  ['Peacock', 'ピーコック'],
  ['Alhena', 'アルヘナ'],
  ['Atria', 'アトリア'],
  ['Alsephina', 'Alsephina'],
  ['Avior', 'アヴィオール'],
  ['Murzim', 'ミルザム'],
  ['Alphard', 'アルファルド'],
  ['Hamal', 'ハマル'],
  ['Diphda', 'ディフダ'],
  ['Polaris', 'ポラリス（現在の北極星）'],
  ['Menkent', 'メンケント'],
  ['Mirach', 'ミラク'],
  ['Saiph', 'サイフ'],
  ['Alpheratz', 'アルフェラッツ'],
  ['Nunki', 'ヌンキ'],
  ['Rasalhague', 'ラス・アルハゲ'],
  ['Kochab', 'コカブ'],
  ['Almach', 'アルマク'],
  ['Tiaki', 'Tiaki'],
  ['Algol', 'アルゴル'],
  ['Denebola', 'デネボラ'],
  ['Muhlifain', 'ムリファイン'],
  ['Suhail', 'スハイル'],
  ['Etamin', 'エルタニン'],
  ['Schedar', 'シェダル'],
  ['Sadr', 'サドル'],
  ['Alphecca', 'アルフェッカ'],
  ['Naos', 'ナオス'],
  ['Aspidiske', 'アスピディスケ'],
  ['Mizar', 'ミザール'],
  ['Caph', 'カフ'],
  ['Larawag', 'Larawag'],
  ['Dschubba', 'ジュバ'],
  ['Merak', 'メラク'],
  ['Algieba', 'アルギエバ'],
  ['Ankaa', 'アンカア'],
  ['Girtab', 'ギルタブ'],
  ['Izar', 'イザール'],
  ['Enif', 'エニフ'],
  ['Mintaka', 'ミンタカ'],
  ['Scheat', 'シェアト'],
  ['Sabik', 'サビク'],
  ['Phecda', 'フェクダ'],
  ['Aludra', 'アルドラ'],
  ['Alderamin', 'アルデラミン'],
  ['Adhara', 'アダラ'],
  ['Markab', 'マルカブ'],
  ['Aljanah', 'アルジェナー'],
  ['Markab', 'マルカブ'],
  ['Nebula', '星雲'],
  ['Pleiades', 'プレアデス星団']
])

const greek = new Map([
  ['Alpha', 'アルファ星'],
  ['Beta', 'ベータ星'],
  ['Gamma', 'ガンマ星'],
  ['Delta', 'デルタ星'],
  ['Epsilon', 'イプシロン星'],
  ['Zeta', 'ジータ星'],
  ['Eta', 'イータ星'],
  ['Theta', 'シータ星'],
  ['Iota', 'イオタ星'],
  ['Kappa', 'カッパ星'],
  ['Lambda', 'ラムダ星'],
  ['Mu', 'ミュー星'],
  ['Nu', 'ニュー星'],
  ['Xi', 'クシー星'],
  ['Omicron', 'オミクロン星'],
  ['Pi', 'パイ星'],
  ['Rho', 'ロー星'],
  ['Sigma', 'シグマ星'],
  ['Tau', 'タウ星'],
  ['Upsilon', 'ウプシロン星'],
  ['Phi', 'ファイ星'],
  ['Chi', 'カイ星'],
  ['Psi', 'プサイ星'],
  ['Omega', 'オメガ星']
])

const constellationGenitive = new Map([
  ['Andromedae', 'Andromeda'],
  ['Antliae', 'Antlia'],
  ['Apodis', 'Apus'],
  ['Aquarii', 'Aquarius'],
  ['Aquilae', 'Aquila'],
  ['Arae', 'Ara'],
  ['Arietis', 'Aries'],
  ['Aurigae', 'Auriga'],
  ['Boötis', 'Boötes'],
  ['Caeli', 'Caelum'],
  ['Camelopardalis', 'Camelopardalis'],
  ['Cancri', 'Cancer'],
  ['Canum Venaticorum', 'Canes Venatici'],
  ['Canis Majoris', 'Canis Major'],
  ['Canis Minoris', 'Canis Minor'],
  ['Capricorni', 'Capricornus'],
  ['Carinae', 'Carina'],
  ['Cassiopeiae', 'Cassiopeia'],
  ['Centauri', 'Centaurus'],
  ['Cephei', 'Cepheus'],
  ['Ceti', 'Cetus'],
  ['Chamaeleontis', 'Chamaeleon'],
  ['Circini', 'Circinus'],
  ['Columbae', 'Columba'],
  ['Comae Berenices', 'Coma Berenices'],
  ['Coronae Australis', 'Corona Australis'],
  ['Coronae Borealis', 'Corona Borealis'],
  ['Corvi', 'Corvus'],
  ['Crateris', 'Crater'],
  ['Crucis', 'Crux'],
  ['Cygni', 'Cygnus'],
  ['Delphini', 'Delphinus'],
  ['Doradus', 'Dorado'],
  ['Draconis', 'Draco'],
  ['Equulei', 'Equuleus'],
  ['Eridani', 'Eridanus'],
  ['Fornacis', 'Fornax'],
  ['Geminorum', 'Gemini'],
  ['Gruis', 'Grus'],
  ['Herculis', 'Hercules'],
  ['Horologii', 'Horologium'],
  ['Hydrae', 'Hydra'],
  ['Hydri', 'Hydrus'],
  ['Indi', 'Indus'],
  ['Lacertae', 'Lacerta'],
  ['Leonis', 'Leo'],
  ['Leonis Minoris', 'Leo Minor'],
  ['Leporis', 'Lepus'],
  ['Librae', 'Libra'],
  ['Lupi', 'Lupus'],
  ['Lyncis', 'Lynx'],
  ['Lyrae', 'Lyra'],
  ['Mensae', 'Mensa'],
  ['Microscopii', 'Microscopium'],
  ['Monocerotis', 'Monoceros'],
  ['Muscae', 'Musca'],
  ['Normae', 'Norma'],
  ['Octantis', 'Octans'],
  ['Ophiuchi', 'Ophiuchus'],
  ['Orionis', 'Orion'],
  ['Pavonis', 'Pavo'],
  ['Pegasi', 'Pegasus'],
  ['Persei', 'Perseus'],
  ['Phoenicis', 'Phoenix'],
  ['Pictoris', 'Pictor'],
  ['Piscium', 'Pisces'],
  ['Piscis Austrini', 'Piscis Austrinus'],
  ['Puppis', 'Puppis'],
  ['Pyxidis', 'Pyxis'],
  ['Reticuli', 'Reticulum'],
  ['Sagittae', 'Sagitta'],
  ['Sagittarii', 'Sagittarius'],
  ['Scorpii', 'Scorpius'],
  ['Sculptoris', 'Sculptor'],
  ['Scuti', 'Scutum'],
  ['Serpentis', 'Serpens'],
  ['Sextantis', 'Sextans'],
  ['Tauri', 'Taurus'],
  ['Telescopii', 'Telescopium'],
  ['Trianguli', 'Triangulum'],
  ['Trianguli Australis', 'Triangulum Australe'],
  ['Tucanae', 'Tucana'],
  ['Ursae Majoris', 'Ursa Major'],
  ['Ursae Minoris', 'Ursa Minor'],
  ['Velorum', 'Vela'],
  ['Virginis', 'Virgo'],
  ['Volantis', 'Volans'],
  ['Vulpeculae', 'Vulpecula']
])

const constellation = new Map([
  ['Andromeda', 'アンドロメダ座'],
  ['Monoceros', 'いっかくじゅう座'],
  ['Sagittarius', 'いて座'],
  ['Delphinus', 'いるか座'],
  ['Indus', 'インディアン座'],
  ['Pisces', 'うお座'],
  ['Lepus', 'うさぎ座'],
  ['Boötes', 'うしかい座'],
  ['Hydra', 'うみへび座'],
  ['Eridanus', 'エリダヌス座'],
  ['Taurus', 'おうし座'],
  ['Canis Major', 'おおいぬ座'],
  ['Lupus', 'おおかみ座'],
  ['Ursa Major', 'おおぐま座'],
  ['Virgo', 'おとめ座'],
  ['Aries', 'おひつじ座'],
  ['Orion', 'オリオン座'],
  ['Pictor', 'がか座'],
  ['Cassiopeia', 'カシオペヤ座'],
  ['Dorado', 'かじき座'],
  ['Cancer', 'かに座'],
  ['Coma Berenices', 'かみのけ座'],
  ['Chamaeleon', 'カメレオン座'],
  ['Corvus', 'からす座'],
  ['Corona Borealis', 'かんむり座'],
  ['Tucana', 'きょしちょう座'],
  ['Auriga', 'ぎょしゃ座'],
  ['Camelopardalis', 'きりん座'],
  ['Pavo', 'くじゃく座'],
  ['Cetus', 'くじら座'],
  ['Cepheus', 'ケフェウス座'],
  ['Centaurus', 'ケンタウルス座'],
  ['Microscopium', 'けんびきょう座'],
  ['Canis Minor', 'こいぬ座'],
  ['Equuleus', 'こうま座'],
  ['Vulpecula', 'こぎつね座'],
  ['Ursa Minor', 'こぐま座'],
  ['Leo Minor', 'こじし座'],
  ['Crater', 'コップ座'],
  ['Lyra', 'こと座'],
  ['Circinus', 'コンパス座'],
  ['Ara', 'さいだん座'],
  ['Scorpius', 'さそり座'],
  ['Triangulum', 'さんかく座'],
  ['Leo', 'しし座'],
  ['Norma', 'じょうぎ座'],
  ['Scutum', 'たて座'],
  ['Caelum', 'ちょうこくぐ座'],
  ['Sculptor', 'ちょうこくしつ座'],
  ['Grus', 'つる座'],
  ['Mensa', 'テーブルさん座'],
  ['Libra', 'てんびん座'],
  ['Lacerta', 'とかげ座'],
  ['Horologium', 'とけい座'],
  ['Volans', 'とびうお座'],
  ['Puppis', 'とも座'],
  ['Musca', 'はえ座'],
  ['Cygnus', 'はくちょう座'],
  ['Octans', 'はちぶんぎ座'],
  ['Columba', 'はと座'],
  ['Apus', 'ふうちょう座'],
  ['Gemini', 'ふたご座'],
  ['Pegasus', 'ペガスス座'],
  ['Serpens', 'へび座'],
  ['Ophiuchus', 'へびつかい座'],
  ['Hercules', 'ヘルクレス座'],
  ['Perseus', 'ペルセウス座'],
  ['Vela', 'ほ座'],
  ['Telescopium', 'ぼうえんきょう座'],
  ['Phoenix', 'ほうおう座'],
  ['Antlia', 'ポンプ座'],
  ['Aquarius', 'みずがめ座'],
  ['Hydrus', 'みずへび座'],
  ['Crux', 'みなみじゅうじ座'],
  ['Piscis Austrinus', 'みなみのうお座'],
  ['Corona Australis', 'みなみのかんむり座'],
  ['Triangulum Australe', 'みなみのさんかく座'],
  ['Sagitta', 'や座'],
  ['Capricornus', 'やぎ座'],
  ['Lynx', 'やまねこ座'],
  ['Pyxis', 'らしんばん座'],
  ['Draco', 'りゅう座'],
  ['Carina', 'りゅうこつ座'],
  ['Canes Venatici', 'りょうけん座'],
  ['Reticulum', 'レチクル座'],
  ['Fornax', 'ろ座'],
  ['Sextans', 'ろくぶんぎ座'],
  ['Aquila', 'わし座']
])

export default {
  data: function () {
    return {
      showMinorNames: false,
      wikipediaData: undefined,
      shareLink: undefined,
      showShareLinkDialog: false,
      copied: false,
      items: []
    }
  },
  computed: {
    selectedObject: function () {
      return this.$store.state.selectedObject
    },
    stelSelectionId: function () {
      return this.$store.state.stel && this.$store.state.stel.selection ? this.$store.state.stel.selection : undefined
    },
    title: function () {
      let title = this.selectedObject ? this.otherNames[0] : 'Selection'
      title = fullWordTranslation.get(title) || title
      let array = title.split(' ').map(str => greek.get(str) || str)
      if (array[0].indexOf('星') !== -1) {
        let constella = array.slice(1).join(' ')
        constella = constellationGenitive.get(constella) || constella
        constella = constellation.get(constella) || constella
        array = constella.split(' ').concat(array[0])
      }
      title = array.map(str => translation.get(str) || constellationGenitive.get(str) || str)
        .map(str => constellation.get(str) || str).join(' ')
      return title
    },
    otherNames: function () {
      return this.selectedObject ? swh.namesForSkySource(this.selectedObject, 26) : undefined
    },
    otherNames1to7: function () {
      return this.otherNames.slice(1, 8)
    },
    otherNames8andMore: function () {
      return this.showMinorNames ? this.otherNames.slice(8) : []
    },
    wikipediaSummary: function () {
      if (!this.wikipediaData) return ''
      const page = this.wikipediaData.query.pages[Object.keys(this.wikipediaData.query.pages)[0]]
      if (!page || !page.extract) return ''
      const wl = '<b><a style="color: #62d1df;" target="_blank" rel="noopener" href="' + this.wikipediaLink + '">wikipedia</a></b></i>'
      return page.extract.replace(/<p>/g, '').replace(/<\/p>/g, '') + '<span class="grey--text text-caption" style="margin-left:auto; margin-right:0;"><i>&nbsp; ' + this.$t('more on {0}', [wl]) + '</span>'
    },
    wikipediaLink: function () {
      const page = this.wikipediaData.query.pages[Object.keys(this.wikipediaData.query.pages)[0]]
      if (!page || !page.extract) return ''
      return 'https://en.wikipedia.org/wiki/' + page.title
    },
    type: function () {
      if (!this.selectedObject) return this.$t('Unknown')
      let morpho = ''
      if (this.selectedObject.model_data && this.selectedObject.model_data.morpho) {
        morpho = swh.nameForGalaxyMorpho(this.selectedObject.model_data.morpho)
        if (morpho) {
          morpho = morpho + ' '
        }
      }
      return morpho + swh.nameForSkySourceType(this.selectedObject.types[0])
    },
    icon: function () {
      return swh.iconForSkySource(this.selectedObject)
    },
    showPointToButton: function () {
      if (!this.$store.state.stel.lock) return true
      if (this.$store.state.stel.lock !== this.$store.state.stel.selection) return true
      return false
    },
    zoomInButtonEnabled: function () {
      if (!this.$store.state.stel.lock || !this.selectedObject) return false
      return true
    },
    zoomOutButtonEnabled: function () {
      if (!this.$store.state.stel.lock || !this.selectedObject) return false
      return true
    },
    extraButtons: function () {
      return swh.selectedObjectExtraButtons
    },
    pluginsSelectedInfoExtraGuiComponents: function () {
      let res = []
      for (const i in this.$stellariumWebPlugins()) {
        const plugin = this.$stellariumWebPlugins()[i]
        if (plugin.selectedInfoExtraGuiComponents) {
          res = res.concat(plugin.selectedInfoExtraGuiComponents)
        }
      }
      return res
    }
  },
  watch: {
    selectedObject: function (s) {
      this.showMinorNames = false
      this.wikipediaData = undefined
      if (!s) {
        if (this.timer) clearInterval(this.timer)
        this.timer = undefined
        return
      }
      var that = this
      that.items = that.computeItems()
      if (that.timer) clearInterval(that.timer)
      that.timer = setInterval(() => {
        that.items = that.computeItems()
      }, 1000)

      swh.getSkySourceSummaryFromWikipedia(s).then(data => {
        that.wikipediaData = data
      }, reason => {
      })
    },
    stelSelectionId: function (s) {
      if (!this.$stel.core.selection) {
        this.$store.commit('setSelectedObject', 0)
        return
      }
      swh.sweObj2SkySource(this.$stel.core.selection).then(res => {
        this.$store.commit('setSelectedObject', res)
      }, err => {
        console.log('Couldn\'t find info for object ' + s + ':' + err)
        this.$store.commit('setSelectedObject', 0)
      })
    },
    showShareLinkDialog: function (b) {
      this.shareLink = swh.getShareLink(this)
    }
  },
  methods: {
    /** memo: 座標の計算などを行う */
    computeItems: function () {
      const obj = this.$stel.core.selection
      if (!obj) return []
      const that = this

      const ret = []

      const addAttr = (key, attr, format) => {
        const v = obj.getInfo(attr)
        if (v && !isNaN(v)) {
          ret.push({
            key: key,
            value: format ? format(v) : v.toString()
          })
        }
      }

      addAttr(that.$t('Magnitude'), 'vmag', this.formatMagnitude)
      addAttr(that.$t('Distance'), 'distance', this.formatDistance)
      if (this.selectedObject.model_data) {
        if (this.selectedObject.model_data.radius) {
          ret.push({
            key: that.$t('Radius'),
            value: this.selectedObject.model_data.radius.toString() + ' Km'
          })
        }
        if (this.selectedObject.model_data.spect_t) {
          ret.push({
            key: that.$t('Spectral Type'),
            value: this.selectedObject.model_data.spect_t
          })
        }
        if (this.selectedObject.model_data.dimx) {
          const dimy = this.selectedObject.model_data.dimy ? this.selectedObject.model_data.dimy : this.selectedObject.model_data.dimx
          ret.push({
            key: that.$t('Size'),
            value: this.selectedObject.model_data.dimx.toString() + '\' x ' + dimy.toString() + '\''
          })
        }
      }
      const formatInt = function (num, padLen) {
        const pad = new Array(1 + padLen).join('0')
        return (pad + num).slice(-pad.length)
      }
      const formatRA = function (a) {
        const raf = that.$stel.a2tf(a, 1)
        return '<div class="radecVal">' + formatInt(raf.hours, 2) + '<span class="radecUnit">h</span>&nbsp;</div><div class="radecVal">' + formatInt(raf.minutes, 2) + '<span class="radecUnit">m</span></div><div class="radecVal">' + formatInt(raf.seconds, 2) + '.' + raf.fraction + '<span class="radecUnit">s</span></div>'
      }
      const formatAz = function (a) {
        const raf = that.$stel.a2af(a, 1)
        return '<div class="radecVal">' + formatInt(raf.degrees < 0 ? raf.degrees + 180 : raf.degrees, 3) + '<span class="radecUnit">°</span></div><div class="radecVal">' + formatInt(raf.arcminutes, 2) + '<span class="radecUnit">\'</span></div><div class="radecVal">' + formatInt(raf.arcseconds, 2) + '.' + raf.fraction + '<span class="radecUnit">"</span></div>'
      }
      const formatDec = function (a) {
        const raf = that.$stel.a2af(a, 1)
        return '<div class="radecVal">' + raf.sign + formatInt(raf.degrees, 2) + '<span class="radecUnit">°</span></div><div class="radecVal">' + formatInt(raf.arcminutes, 2) + '<span class="radecUnit">\'</span></div><div class="radecVal">' + formatInt(raf.arcseconds, 2) + '.' + raf.fraction + '<span class="radecUnit">"</span></div>'
      }
      const posCIRS = this.$stel.convertFrame(this.$stel.core.observer, 'ICRF', 'JNOW', obj.getInfo('radec'))
      const radecCIRS = this.$stel.c2s(posCIRS)
      const raCIRS = this.$stel.anp(radecCIRS[0])
      const decCIRS = this.$stel.anpm(radecCIRS[1])
      ret.push({
        key: that.$t('Ra/Dec'),
        value: formatRA(raCIRS) + '&nbsp;&nbsp;&nbsp;' + formatDec(decCIRS)
      })
      const azalt = this.$stel.c2s(this.$stel.convertFrame(this.$stel.core.observer, 'ICRF', 'OBSERVED', obj.getInfo('radec')))
      const az = this.$stel.anp(azalt[0])
      const alt = this.$stel.anpm(azalt[1])
      ret.push({
        key: that.$t('Az/Alt'),
        value: formatAz(az) + '&nbsp;&nbsp;&nbsp;' + formatDec(alt)
      })
      addAttr(that.$t('Phase'), 'phase', this.formatPhase)
      const vis = obj.computeVisibility()
      let str = ''
      if (vis.length === 0) {
        str = that.$t('Not visible tonight')
      } else if (vis[0].rise === null) {
        str = that.$t('Always visible tonight')
      } else {
        str = that.$t('Rise: {0}&nbsp;&nbsp;&nbsp; Set: {1}', [this.formatTime(vis[0].rise), this.formatTime(vis[0].set)])
      }
      ret.push({
        key: that.$t('Visibility'),
        value: str
      })
      return ret
    },
    formatPhase: function (v) {
      return (v * 100).toFixed(0) + '%'
    },
    formatMagnitude: function (v) {
      if (!v) {
        return 'Unknown'
      }
      return v.toFixed(2)
    },
    formatDistance: function (d) {
      // d is in AU
      if (!d) {
        return 'NAN'
      }
      const ly = d * swh.astroConstants.ERFA_AULT / swh.astroConstants.ERFA_DAYSEC / swh.astroConstants.ERFA_DJY
      if (ly >= 0.1) {
        return ly.toFixed(2) + '<span class="radecUnit"> 光年</span>'
      }
      if (d >= 0.1) {
        return d.toFixed(2) + '<span class="radecUnit"> AU</span>'
      }
      const meter = d * swh.astroConstants.ERFA_DAU
      if (meter >= 1000) {
        return (meter / 1000).toFixed(2) + '<span class="radecUnit"> km</span>'
      }
      return meter.toFixed(2) + '<span class="radecUnit"> m</span>'
    },
    formatTime: function (jdm) {
      var d = new Date()
      d.setMJD(jdm)
      const utc = new Moment(d)
      utc.utcOffset(this.$store.state.stel.utcoffset)
      return utc.format('HH:mm')
    },
    unselect: function () {
      this.$stel.core.selection = 0
    },
    lockToSelection: function () {
      if (this.$stel.core.selection) {
        this.$stel.pointAndLock(this.$stel.core.selection, 0.5)
      }
    },
    zoomInButtonClicked: function () {
      const currentFov = this.$store.state.stel.fov * 180 / Math.PI
      this.$stel.zoomTo(currentFov * 0.3 * Math.PI / 180, 0.4)
      const that = this
      this.zoomTimeout = setTimeout(_ => {
        that.zoomInButtonClicked()
      }, 300)
    },
    zoomOutButtonClicked: function () {
      const currentFov = this.$store.state.stel.fov * 180 / Math.PI
      this.$stel.zoomTo(currentFov * 3 * Math.PI / 180, 0.6)
      const that = this
      this.zoomTimeout = setTimeout(_ => {
        that.zoomOutButtonClicked()
      }, 200)
    },
    stopZoom: function () {
      if (this.zoomTimeout) {
        clearTimeout(this.zoomTimeout)
        this.zoomTimeout = undefined
      }
    },
    extraButtonClicked: function (btn) {
      btn.callback()
    },
    copyLink: function () {
      const input = document.querySelector('#link_inputid')
      input.focus()
      input.select()
      this.copied = document.execCommand('copy')
      window.getSelection().removeAllRanges()
      this.showShareLinkDialog = false
    }
  },
  mounted: function () {
    const that = this
    window.addEventListener('mouseup', function (event) {
      that.stopZoom()
    })
  }
}
</script>

<style>
.bt_disabled {
  filter: opacity(0.2);
}

.radecVal {
  display: inline-block;
  font-family: monospace;
  padding-right: 2px;
  font-size: 13px;
  font-weight: bold;
}

.radecUnit {
  color: #dddddd;
  font-weight: normal
}
</style>
