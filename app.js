/**
 * app.js connects index.html and env.js together
 * User activty is required for Web Bluetooth the make the connection
 */
let bleSwitch = document.querySelector('#bleSwitch');

bleSwitch.addEventListener('click',function(){
  console.log('new switch click, connect');
  env.connect()
      .then(() => console.log('connected'))
      .catch(error => { console.log('connect error!');
    });
});
