<!--位置情報-->
// Stellarium Web - Copyright (c) 2022 - Stellarium Labs SRL
//
// This program is licensed under the terms of the GNU AGPL v3, or
// alternatively under a commercial licence.
//
// The terms of the AGPL v3 license can be found in the main directory of this
// repository.

<template>
<v-dialog max-width="600" v-model="$store.state.showLocationDialog">
  <v-container v-if="$store.state.showLocationDialog" class="secondary white--text">
    <v-card color="secondary" flat>
      <v-btn @click="requestPermission">加速度センサを使用</v-btn>
      <v-switch :label="$t('Use Autolocation')" v-model="useAutoLocation"></v-switch>
    </v-card>
    <location-mgr v-on:locationSelected="setLocation" :knownLocations="[]" :startLocation="$store.state.currentLocation" :realLocation="$store.state.autoDetectedLocation"></location-mgr>
  </v-container>
</v-dialog>
</template>

<script>
import LocationMgr from '@/components/location-mgr.vue'

export default {
  data: function () {
    return {
    }
  },
  computed: {
    useAutoLocation: {
      get: function () {
        return this.$store.state.useAutoLocation
      },
      set: function (b) {
        this.$store.commit('setUseAutoLocation', b)
      }
    }
  },
  mounted: function () {
  },
  methods: {
    requestPermission () {
      this.$store.dispatch('requestPermission')
    },
    setLocation: function (loc) {
      this.$store.commit('setCurrentLocation', loc)
      this.$store.commit('toggleBool', 'showLocationDialog')
    }
  },
  components: { LocationMgr }
}
</script>

<style>
</style>
