const { AudioServer } = require('../');

AudioServer.setDebugLog(true);

const audioServer = new AudioServer();

const logDevice = (device) => {
  console.log('---------------');
  for (let prop in device) {
    console.log(`${prop}:`, device[prop]);
  }
}

const main = async () => {
  audioServer.getDevices().outputDevices.forEach(logDevice);
  audioServer.getDevices().inputDevices.forEach(logDevice);
  console.log('-------')

  console.log('API:', audioServer.getApi());
}
main();
