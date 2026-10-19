// Exercise the actual page script with a minimal DOM and controlled API responses.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync('espnow_rx/web/index.html','utf8');
class Element {
 constructor(){this.children=[];this.hidden=false;this.textContent='';this.className='';}
 append(e){this.children.push(e);}replaceChildren(){this.children=[];}
 get firstChild(){return this.children[0];}addEventListener(){}
}
const elements=new Map();
let fail=false;
let data={received:10,alarm_count:0,last_contact_ms:100,channel:6,ip:'192.168.1.176',version:'0.3.0',uptime_ms:10000,wifi_rssi:-70,duplicates:0,errors:0,queue_drops:0,radio_ready:true,channel_ok:true,node_online:true,sensor_state:2,distance_mm:400,sample_age_ms:100,events:[]};
const context=vm.createContext({document:{getElementById(id){if(!elements.has(id))elements.set(id,new Element());return elements.get(id);},createElement(){return new Element();}},
 fetch:async()=>{if(fail)throw Error('offline');return {ok:true,json:async()=>data};},
 AbortSignal:{timeout:()=>null},setInterval:()=>0});
(async()=>{
 vm.runInContext(html.split('<script>')[1].split('</script>')[0],context);
 await new Promise(resolve=>setImmediate(resolve));
 assert.equal(elements.get('status').textContent,'Zona libera');
 assert.equal(elements.get('events').hidden,true);
 data={...data,sensor_state:3,distance_mm:100,alarm_count:1,events:[{node:1,seq:9,age_ms:500,kind:4}]};await context.update();
 assert.equal(elements.get('status').textContent,'Ostacolo presente');
 assert.equal(elements.get('rows').children.length,1);
 data={...data,sensor_state:2,distance_mm:400};await context.update();
 assert.equal(elements.get('status').textContent,'Zona libera'); // history is not current occupancy
 data={...data,sample_age_ms:2000};await context.update();assert.equal(elements.get('status').textContent,'Misura non aggiornata');
 data={...data,sample_age_ms:100,sensor_state:1,distance_mm:-1};await context.update();assert.equal(elements.get('status').textContent,'Eco non disponibile');
 data={...data,node_online:false};await context.update();
 assert.equal(elements.get('status').textContent,'Nodo senza contatto recente');
 fail=true;await context.update();
 assert.equal(elements.get('status').textContent,'Aggiornamento interrotto');
 assert.equal(elements.get('banner').className,'banner warn');
 console.log('PASS dashboard: stato nodo, allarme, indisponibilità e dati obsoleti');
})().catch(e=>{console.error(e);process.exitCode=1;});
