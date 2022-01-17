<template>
  <v-container fluid>
    <v-toolbar dense floating class=mb-0>
      <v-btn color="purple" icon @click="hidden = !hidden">
        <v-icon>tune</v-icon>
      </v-btn>
      <v-btn
        icon
        color="primary"
        :loading="tare_load"
        :disabled="tare_load"
        @click="tare"
      >
        <v-icon>autorenew</v-icon>
      </v-btn>
      <v-btn color="green" icon @click="loadcell_resume">
        <v-icon>play_arrow</v-icon>
      </v-btn>
      <v-btn color="warning" icon @click="loadcell_pause">
        <v-icon>pause</v-icon>
      </v-btn>
    </v-toolbar>
    <v-text-field
      style="position:absolute;z-index:10;max-width:85px"
      class="pa-0 ma-0 pl-4"
      dense
      :placeholder="scale"
      v-show="!hidden"
      color="purple"
      v-model="weight"
      @keypress.enter="calibrate">
    </v-text-field>
    <v-card-text class=pt-0>
      <div id="tester" ref='tester'></div>
    </v-card-text>
  
  </v-container>
</template>

<script>
export default {
  data () {
    return {
      timer: null,
      loader: null,
      cal_load: false,
      tare_load: false,
      hidden: true,
      scale: 0,
      weight: '',
      trace1:{
      x: [1, 2, 3, 4],
      y: [10, 15, 13, 17],
      xaxis: 'x1',
      yaxis: 'y1',
      name: 'P',
      mode: 'lines'
      },
      trace2:{
      x: [1, 2, 3, 4],
      y: [0, 0, 0, 1],
      xaxis: 'x1',
      yaxis: 'y1',
      name: 'I',
      mode: 'lines'
      },
      trace3:{
      x: [1, 2, 3, 4],
      y: [12, 9, 15, 12],
      xaxis: 'x2',
      yaxis: 'y2',
      name: 'D',
      mode: 'lines'
      }
    }
  },
  computed: {
    get_chart_value () {
      return this.$store.state.chart_value
    }
  },
  methods: {
    updateData: function () {
      this.$store.dispatch('update_chart_value')
    },
    tare: function () {
      this.tare_load = true
      this.$ajax
        .get('/api/v1/loadcell/tare')
        .then(data => {
          if (data.data.created) {
            this.tare_load = !data.data.created
            this.timer = setInterval(this.updateData, 1000)
          }
        })
        .catch(error => {
          console.log(error)
        })
    },
    calibrate: function () {
      this.$ajax
        .post('/api/v1/loadcell/calibrate', {
          weight: parseInt(this.weight)
        })
        .then(data => {
          this.scale = data.data.scale
          console.log('Loadcell Calibrated')
        })
        .catch(error => {
          console.log(error)
        })
    },
    loadcell_resume: function () {
      this.trace1.y[0] +=1
      Plotly.redraw(this.$refs.tester);
      this.$ajax
        .get('/api/v1/loadcell/resume')
        .then(data => {
          this.timer = setInterval(this.updateData, 1000)
          console.log(data.data)
        })
        .catch(error => {
          console.log(error)
        })
    },
    loadcell_pause: function () {
      this.$ajax
        .get('/api/v1/loadcell/pause')
        .then(data => {
          clearInterval(this.timer)
          console.log(data.data)
        })
        .catch(error => {
          console.log(error)
        })
    },
    staticfire: function () {
      this.$ajax
        .get('/api/v1/staticfire')
        .then(data => {
          console.log(data.data)
        })
        .catch(error => {
          console.log(error)
        })
    }
  },
  mounted () {
    clearInterval(this.timer)
    this.$ajax
      .get('/api/v1/loadcell/init')
      .then(data => {
        this.scale = data.data.scale
      })
      .catch(error => {
        console.log(error)
      })
  
    var traces = [this.trace1, this.trace2, this.trace3];

    var layout = {
      title: 'Live ploting',
      plot_bgcolor:'#212121',
      paper_bgcolor:'#212121',
      grid: {rows: 2, columns: 1, pattern: 'independent'},
      font: {
        color: 'white'
      },
      xaxis: {
        title: 'Time [s]'
      },
      yaxis: {
        title: 'PID konstants'
      }
    };
    var config = {responsive: true}
    Plotly.newPlot(this.$refs.tester, traces, layout,config)
  },
  destroyed: function () {
    clearInterval(this.timer)
  }
}
</script>
