// Stellarium Web - Copyright (c) 2022 - Stellarium Labs SRL
//
// This program is licensed under the terms of the GNU AGPL v3, or
// alternatively under a commercial licence.
//
// The terms of the AGPL v3 license can be found in the main directory of this
// repository.

<template>

  <div class="secondary" style="position:absolute; width: 100%; height: 100%;">
    <v-container style="width: 100%; height: 100%;">
      <HoshizoraHeader></HoshizoraHeader>
      <v-row align-content="center" style="height: 500px">
        <v-col align="center">
          星空プラネタリウム ロード中
          <v-progress-circular indeterminate class="mx-3"></v-progress-circular>
        </v-col>
      </v-row>
    </v-container>
  </div>

</template>

<script>
export default {}
</script>

<style>
</style>
