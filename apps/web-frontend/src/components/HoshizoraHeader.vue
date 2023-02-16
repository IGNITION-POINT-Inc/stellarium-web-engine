<template>
  <div class="Header" ref="contents">
    <div class="container">
      <p class="logo">
        <a href="/home"></a>
        <img src="@/assets/images/common/logo.svg" alt=""/>
      </p>
      <p class="menu" @click="modalMenuOpen()" :class="{'menu-open':this.modal}"><span
        class="line1"></span><span class="line2"></span></p>
      <div class="modal" :class="{'modal-open':this.modal}">
        <div class="bg" @click="modalMenuClose()"></div>
        <div class="modal-container">
          <ul class="navis-modal">
            <li class="bt" @click="modalMenuClose()"><a href="/sketch"></a>
                <span class="txt">星空スケッチ</span><span class="arw"><img src="@/assets/images/common/arw.svg" alt=""/></span>
            </li>
            <li class="bt" @click="modalMenuClose()"><a href="/spot"></a>
                <span class="txt">星空スポット</span><span class="arw"><img src="@/assets/images/common/arw.svg" alt=""/></span>
            </li>
            <li class="bt" @click="modalMenuClose()">
              <a href="/story"></a>
                <span class="txt">星空ストーリー</span><span class="arw"><img src="@/assets/images/common/arw.svg" alt=""/></span>
            </li>
            <li class="bt" @click="modalMenuClose()"><a href="#"></a><span class="txt">星空プラネタリウム</span><span
              class="arw"><img src="@/assets/images/common/arw.svg" alt=""/></span></li>
          </ul>
          <div>
            <v-row align-content="center" class="mt-3">
              <v-col align="center">
                <button @click="toggleStoreValue()">© データクレジット</button>
              </v-col>
            </v-row>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import Util from '@/plugins/util'
import { TweenMax, Expo } from 'gsap'

export default {
  data () {
    return {
      stageW: 0,
      stageH: 0,
      timer_resize: 0,

      targetDevice: '',

      modal: false
    }
  },

  mounted () {
    this.init()
  },
  beforeDestroy: function () {
  },
  methods: {
    init () {
      TweenMax.to(this.$refs.contents, 0, {
        alpha: 0,
        delay: 0,
        overwrite: 1,
        ease: Expo.easeOut
      })

      this.addAc()
    },
    toggleStoreValue: function () {
      this.$store.commit('toggleBool', 'showDataCreditsDialog')
    },
    addAc () {
      window.addEventListener('resize', this.resizeEv)
      this.resizeAc()

      TweenMax.to(this.$refs.contents, 0, {
        alpha: 0,
        delay: 0,
        overwrite: 1,
        ease: Expo.easeOut
      })
      TweenMax.to(this.$refs.contents, 0.6, {
        alpha: 1,
        delay: 0.3,
        overwrite: 1,
        ease: Expo.easeOut
      })
    },
    removeAc () {
      window.removeEventListener('resize', this.resizeEv)
      TweenMax.to(this.$refs.contents, 0.6, {
        alpha: 0,
        delay: 0,
        overwrite: 1,
        ease: Expo.easeOut
      })
    },
    modalMenuOpen () {
      if (this.modal) {
        this.modal = false
        TweenMax.to(document.querySelector('.Header .modal'), 0, {
          alpha: 0,
          delay: 0,
          overwrite: 1,
          ease: Expo.easeOut
        })
      } else {
        this.modal = true
        TweenMax.to(document.querySelector('.Header .modal'), 0, {
          alpha: 0,
          delay: 0,
          overwrite: 1,
          ease: Expo.easeOut
        })
        TweenMax.to(document.querySelector('.Header .modal'), 0.6, {
          alpha: 1,
          delay: 0,
          overwrite: 1,
          ease: Expo.easeOut
        })
      }
      this.resizeAc()
    },
    modalMenuClose () {
      this.modal = false
      TweenMax.to(document.querySelector('.Header .modal'), 0, {
        alpha: 0,
        delay: 0,
        overwrite: 1,
        ease: Expo.easeOut
      })
      this.resizeAc()
    },

    // ---------------------------------------------
    // リサイズ
    // ---------------------------------------------
    resizeEv () {
    },
    resizeAc () {
      var __this = this
      this.stageW = window.innerWidth
      this.stageH = window.innerHeight

      this.targetDevice = Util.checkUA()

      if (this.timer_resize) {
        clearTimeout(this.timer_resize)
      }
      this.timer_resize = setTimeout(function () {
        // modal
        if (__this.modal) {
          Element.getElementsByTagName('body').css({
            height: '100vh',
            'overflow-y': 'hidden'
          })
          if (__this.stageH > Element.getElementsByTagName('.Header .modal-container').height()) {
            Element.getElementsByTagName('.Header .modal').removeClass('modal-static')
          } else {
            Element.getElementsByTagName('.Header .modal').addClass('modal-static')
          }
        } else {
          Element.getElementsByTagName('body').css({
            height: 'auto',
            'overflow-y': 'scroll'
          })
        }
      }
      , 10)
    }
  },

  computed: {}

}
</script>

<style lang="scss" scoped>
@import "../assets/scss/hoshizoraConfig";
@import "../assets/scss/commons/hoshizoraHeader";

ul {
  list-style: none;
}

</style>
