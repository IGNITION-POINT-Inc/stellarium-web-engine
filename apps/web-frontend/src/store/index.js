// Stellarium Web - Copyright (c) 2022 - Stellarium Labs SRL
//
// This program is licensed under the terms of the GNU AGPL v3, or
// alternatively under a commercial licence.
//
// The terms of the AGPL v3 license can be found in the main directory of this
// repository.
/* eslint-disable */
import Vue from 'vue'
import Vuex from 'vuex'
import _ from 'lodash'

Vue.use(Vuex)

let handleDeviceOrientation = null
const createStore = () => {
  var pluginsModules = {}
  for (const i in Vue.SWPlugins) {
    const plugin = Vue.SWPlugins[i]
    if (plugin.storeModule) {
      console.log('Register store module for plugin: ' + plugin.name)
      pluginsModules[plugin.name] = plugin.storeModule
    }
  }

  return new Vuex.Store({
    modules: pluginsModules,

    state: {
      stel: null,
      initComplete: false,

      showNavigationDrawer: false,
      showDataCreditsDialog: false,
      showViewSettingsDialog: false,
      showPlanetsVisibilityDialog: false,
      showLocationDialog: false,
      selectedObject: undefined,

      showSidePanel: false,

      showMainToolBar: true,
      showLocationButton: true,
      showTimeButtons: true,
      showObservingPanelTabsButtons: true,
      showSelectedInfoButtons: true,
      showFPS: false,
      showEquatorialJ2000GridButton: false,

      fullscreen: false,
      nightmode: false,
      wasmSupport: true,

      autoDetectedLocation: {
        short_name: 'Unknown',
        country: 'Unknown',
        street_address: '',
        lat: 0,
        lng: 0,
        alt: 0,
        accuracy: 5000
      },

      currentLocation: {
        short_name: 'Unknown',
        country: 'Unknown',
        street_address: '',
        lat: 0,
        lng: 0,
        alt: 0,
        accuracy: 5000
      },

      useAutoLocation: true,
      accelState: false
    },
    actions: {
      requestPermission({ dispatch }) {
        if (typeof DeviceOrientationEvent.requestPermission === 'function') {
          DeviceOrientationEvent.requestPermission()
            .then(permissionState => {
              if (permissionState === 'granted') {
                handleDeviceOrientation = (event) => dispatch('handleOrientation', event);
                window.addEventListener('deviceorientation', handleDeviceOrientation);
                this.state.accelState = true
              }
            })
            .catch(console.error)
        } else {
          handleDeviceOrientation = (event) => dispatch('handleOrientation', event);
          window.addEventListener('deviceorientation', handleDeviceOrientation);
          this.state.accelState = true
        }
      },
      accelOff() {

        window.removeEventListener('deviceorientation', handleDeviceOrientation);
        this.state.accelState = false

      },
      handleOrientation(context, event) {      
        const {absolute, alpha, beta, gamma, webkitCompassHeading } = event;
        //console.log("data", alpha, beta, gamma, webkitCompassHeading)
        const $stel = Vue.prototype.$stel;

        let compassHeading;
        if (webkitCompassHeading) {
          // Safari
          compassHeading = webkitCompassHeading;
          
                    $stel.core.observer.yaw = compassHeading ? Number(compassHeading) * Math.PI / 180 : 0;
          $stel.core.observer.pitch = beta ? Number(beta) * Math.PI / 180 - 90 * Math.PI / 180 : 0;
          
        } else if (alpha !== null) {
          // Other browsers
          var degtorad = Math.PI / 180; /*  度° ↔ ラジアン 間の換算用  */

          var _x = beta  ? beta  * degtorad : 0; // β 値
          var _y = gamma ? gamma * degtorad : 0; // γ 値
          var _z = alpha ? alpha * degtorad : 0; // α 値
          
          var cX = Math.cos( _x );
          var cY = Math.cos( _y );
          var cZ = Math.cos( _z );
          var sX = Math.sin( _x );
          var sY = Math.sin( _y );
          var sZ = Math.sin( _z );
          
          /*  V の x , y 成分を計算する  */
          var Vx = - cZ * sY - sZ * sX * cY;
          var Vy = - sZ * sY + cZ * sX * cY;
          
          /*  コンパスの向きを計算する  */
          compassHeading = Math.atan( Vx / Vy );
          
          /*  コンパスの向きを， 0 以上 2 π 未満に換算する  */
          if( Vy < 0 ) {
            compassHeading += Math.PI;
          } else if( Vx < 0 ) {
            compassHeading += 2 * Math.PI;
          }

       	  //yawとpitchを決定
          $stel.core.observer.yaw = compassHeading ? Number(compassHeading) : 0;          
          $stel.core.observer.pitch = beta ? Number(beta) * Math.PI / 180 - 90* Math.PI / 180 : 0;
        } 
      }
    },
    mutations: {
      replaceStelWebEngine(state, newTree) {
        // mutate StelWebEngine state
        state.stel = newTree
      },
      toggleBool(state, varName) {
        _.set(state, varName, !_.get(state, varName))
      },
      setValue(state, { varName, newValue }) {
        _.set(state, varName, newValue)
      },
      setAutoDetectedLocation(state, newValue) {
        state.autoDetectedLocation = { ...newValue }
        if (state.useAutoLocation) {
          state.currentLocation = { ...newValue }
        }
      },
      setUseAutoLocation(state, newValue) {
        state.useAutoLocation = newValue
        if (newValue) {
          state.currentLocation = { ...state.autoDetectedLocation }
        }
      },
      setCurrentLocation(state, newValue) {
        state.useAutoLocation = false
        state.currentLocation = { ...newValue }
      },
      setSelectedObject(state, newValue) {
        state.selectedObject = newValue
      }
    }
  })
}

export default createStore
