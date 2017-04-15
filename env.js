(function() {
  'use strict';

  class Env {

    /**
     * customize your project here to reflect the uuid of your service and characteristics.
     */
    constructor() {
        this.deviceName = 'env_sensor';
        this.serviceUUID = "19B10040-E8F2-537E-4F6C-D104768A1214";
        this.characteristic1UUID = "19B10041-E8F2-537E-4F6C-D104768A1215";
        this.device = null;
        this.server = null;
        // The cache allows us to hold on to characeristics for access in response to user commands
        this._characteristics = new Map();
    }

    connect(){
        return navigator.bluetooth.requestDevice({
         filters: [{
          services:[this.serviceUUID]
         }]
        })
        .then(device => {
            this.device = device;
            return device.gatt.connect();
        })
        .then(server => {
            this.server = server;
            return Promise.all([
              server.getPrimaryService(this.serviceUUID)
              .then(service=>{
                return Promise.all([
                  this._cacheCharacteristic(service, this.characteristic1UUID),
                  // this._cacheCharacteristic(service, 'uuidCharacteristic2Here'),
                ])
              })
            ]);
        })
    }

  _cacheCharacteristic(service, characteristicUuid){
    return service.getCharacteristic(characteristicUuid)
    .then(characteristic => {
      this._characteristics.set(characteristicUuid, characteristic);
    });
  }

 _readCharacteristic(characteristicUuid) {
   let characteristic = this._characteristics.get(characteristicUuid);
   return characteristic.readValue()
   .then(value => {
     value - value.buffer ? value : new DataView(value);
     return value;
   });

 }
 _writeCharacteristic(characteristicUuid, value){
   let characteristic = this._characteristics.get(characteristicUuid);
   return characteristic.writeValue(value);
 }
}

window.env = new Env();

})();
