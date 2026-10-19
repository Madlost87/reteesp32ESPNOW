const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const elements=new Map();const el=id=>{if(!elements.has(id))elements.set(id,{textContent:'',value:'',disabled:false,href:'',addEventListener(type,fn){this[type]=fn;}});return elements.get(id);};
let data={role:'TX',version:'0.5.0',state:0,sent_seq:0,received_count:0,received_seq:0,received_age_ms:-1,sent_text:'',received_text:''},fail=false,posts=0;
const context=vm.createContext({document:{getElementById:el},setInterval(){},AbortSignal:{timeout(){}},fetch:async(url,options)=>{if(fail)throw Error('offline');if(options.method==='POST'){assert.equal(options.headers['X-Radio-Test'],'1');posts++;data.state=1;data.sent_text=options.body;}return {ok:true,json:async()=>data};}});
(async()=>{
const script=fs.readFileSync('shared/web/radio.html','utf8').split('<script>')[1].split('</script>')[0];vm.runInContext(script,context);await new Promise(r=>setImmediate(r));
assert.equal(el('send').disabled,false);el('message').value='Ciao <b>è</b>';await el('form').submit({preventDefault(){}});assert.equal(posts,1);assert.equal(el('send').disabled,true);
data={...data,state:3,sent_seq:10,received_count:1,received_seq:20,received_age_ms:200,received_text:'Ciao <b>è</b>'};await context.refresh();assert.match(el('result').textContent,/Messaggio ricevuto/);assert.match(el('last').textContent,/RX.*20/);
assert.equal(el('received-text').textContent,'Ciao <b>è</b>');assert.equal(el('sent-text').textContent,'Ciao <b>è</b>');assert.equal(el('message').value,'');
data={...data,role:'RX',state:4};await context.refresh();assert.match(el('result').textContent,/Nessuna conferma/);assert.match(el('peer').href,/cantiere-tx/);
fail=true;await context.refresh();assert.equal(el('send').disabled,true);assert.match(el('connection').textContent,/interrotta/);
console.log('PASS pagina radio: invio, conferma, ricezione, timeout e disconnessione');
})().catch(e=>{console.error(e);process.exitCode=1;});
