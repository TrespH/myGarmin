import { Component, ViewChild, ElementRef } from '@angular/core';
import { NavController, Platform, ToastController, AlertController } from 'ionic-angular';
import { BluetoothSerial } from '@ionic-native/bluetooth-serial';
import { Subscription } from 'rxjs/Subscription';

declare var google;

@Component({
  selector: 'page-home',
  templateUrl: 'home.html'
})
export class HomePage {
  @ViewChild('map') mapContainer: ElementRef;
  map: any;
  currentMapTrack = null;
  isTracking = false;
  trackedRoute = [];
  //previousTracks = [];
  positionSubscription: Subscription;
  alertPresent: any = false;
  alert: any;
  latitude: any;
  longitude: any;
  constructor(
    public navCtrl: NavController,
    private bluetoothSerial: BluetoothSerial,
    private toastCtrl: ToastController,
    private plt: Platform,
    public alertCtrl: AlertController,
  ) {
    this.bluetoothSerial.enable()
      .then(() => {
        this.bluetoothSerial.connect("00:21:13:04:66:50")
          .subscribe((data) => {
            let toast = this.toastCtrl.create({
              message: 'Bluetooth Connesso',
              duration: 3000,
              position: 'bottom'
            });
            toast.present();
          }, error => {
            let toast = this.toastCtrl.create({
              message: 'Bluetooth Dismesso',
              duration: 3000,
              position: 'bottom'
            });
            toast.present();
          });
        this.bluetoothSerial.subscribeRawData().subscribe((data) => { this.read(); });
      });
  }

  ionViewDidEnter() {
    this.initializeBackButtonCustomHandler();
  }

  public initializeBackButtonCustomHandler(): void {
    this.plt.registerBackButtonAction(() => {
      if (!this.alertPresent) {
        this.alertPresent = true;
        this.alert = this.alertCtrl.create({
          title: 'Chiusura App',
          message: 'La registrazione verrà eliminata',
          buttons: [{
            text: 'Annulla',
            role: 'cancel',
            handler: () => {
              this.alertPresent = false;
              console.log('Chiusura annullata!');
            }
          }, {
            text: 'Chiudi App',
            handler: () => {
              this.plt.exitApp();
            }
          }]
        });
        this.alert.present();
      } else {
        this.alert.dismiss();
        this.alertPresent = false;
      }
    }, 10);
  }

  ionViewDidLoad() {
    this.plt.ready().then(() => {
      //this.loadHistoricRoutes();
      let latLng = new google.maps.LatLng(41.708, 15.714);
      let mapOptions = {
        center: latLng,
        zoom: 13,
        mapTypeId: google.maps.MapTypeId.ROADMAP,
        mapTypeControl: false,
      }
      this.map = new google.maps.Map(document.getElementById("map"), mapOptions);
    });
  }

  read() {
    this.bluetoothSerial.read()
      .then((data) => {
        if (data != null && data != "" && data != "NaN" && data != 'undefined') {
          var lat = "";
          var lon = "";
          for (let i = 0; i < 9; i++) lat += data[i];
          for (let i = 9; i < 18; i++) lon += data[i];
          //alert("read data : " + latitude + " - " + longitude);
          this.latitude = +lat;
          this.longitude = +lon;
          if (isNaN(this.latitude) || isNaN(this.longitude)) {
            this.latitude = 0;
            this.longitude = 0;
          }
          else {
            //alert("Coo: " + this.latitude + " - " + this.longitude);
            this.trackedRoute.push({ lat: this.latitude, lng: this.longitude });
            let latLng = new google.maps.LatLng(this.latitude, this.longitude);
            this.map.setCenter(latLng);
            this.redrawPath(this.trackedRoute);
          }
        }
      });
  }



  redrawPath(path) {
    if (this.currentMapTrack) this.currentMapTrack.setMap(null);
    if (path.length > 1) {
      this.currentMapTrack = new google.maps.Polyline({
        path: path,
        geodesic: true,
        strokeColor: '#f44242',
        strokeOpacity: 1.0,
        strokeWeight: 3
      });
      this.currentMapTrack.setMap(this.map);
    }
  }

}
