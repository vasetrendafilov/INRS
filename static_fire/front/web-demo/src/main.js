import Vue from 'vue'
import './plugins/vuetify'
import App from './App.vue'
import router from './router'
import axios from 'axios'
import store from './store'
//import { Plotly }  from "vue-plotly"

Vue.config.productionTip = false

Vue.prototype.$ajax = axios

//Vue.component("Plotly", Plotly)

new Vue({
  router,
  store,
  render: h => h(App)
}).$mount('#app')
