var pollhttp;
var polltmr=null;
var pollwait=null;
var divcur=new Array();
var divcon=new Array();
var divinfo=new Array();
var nodename=new Array();
var nodeloc=new Array();
var nodegrp=new Array();
var nodegrpgrp=new Array();
var nodepoll=new Array();
var nodepollpoll=new Array();
var astate = false;
var needsave=0;
var nodecount;
var nodeid;
if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
  pollhttp=new XMLHttpRequest();
} else {
  pollhttp=new ActiveXObject("Microsoft.XMLHTTP");
}
var curnode=null;
function SaveNode(newid)
{
  var i=newid.substr(4);
  var c=-1;
  if (curnode != null)
    c=curnode.substr(4);
  document.getElementById('divconfigcur').innerHTML=divcur[i];
  document.getElementById('divconfigcon').innerHTML=divcon[i];
  document.getElementById('divconfiginfo').innerHTML=divinfo[i];
  if (c != -1) {
    if (i != c)
      document.getElementById(curnode).className='normal';
  } else {
    document.getElementById('configcur').disabled=false;
    document.getElementById('configcon').disabled=false;
    document.getElementById('configinfo').disabled=false;
  }
  curnode = newid;
  DoNodeHelp();
  document.getElementById(curnode).className='click';
  return true;
}
function DisplayNode(n)
{
  return true;
}
function PollTimeout()
{
  pollhttp.abort();
  Poll();
}
function PollReply()
{
  var xml;
  var elem;

  if (pollhttp.readyState == 4 && pollhttp.status == 200) {
    clearTimeout(pollwait);
    xml = pollhttp.responseXML;
    elem = xml.getElementsByTagName('poll');
    if (elem[0].getAttribute('homeid') != document.getElementById('homeid').value)
      document.getElementById('homeid').value = elem[0].getAttribute('homeid');
    if (elem[0].getAttribute('nodecount') != nodecount) {
      nodecount = elem[0].getAttribute('nodecount');
      document.getElementById('nodecount').value = nodecount;
    }
    if (elem[0].getAttribute('nodeid') != nodeid) {
      nodeid = elem[0].getAttribute('nodeid');
    }
    if (elem[0].getAttribute('cmode') != document.getElementById('cmode').value)
      document.getElementById('cmode').value = elem[0].getAttribute('cmode');
    if (elem[0].getAttribute('save') != needsave) {
	needsave = elem[0].getAttribute('save');
	span = document.getElementById('saveinfo');
	if (needsave == '1') {
	  span.style.display = 'block';
	} else {
	  span.style.display = 'none';
	}
    }
    elem = xml.getElementsByTagName('admin');
    if (elem[0].getAttribute('active') == 'true') {
      if (!astate) {
	document.AdmPost.admgo.style.display = 'none';
	document.AdmPost.admcan.style.display = 'inline';    
	document.AdmPost.adminops.disabled = true;
	astate = true;
      }
    } else if (elem[0].getAttribute('active') == 'false') {
      if (astate) {
	document.AdmPost.admgo.style.display = 'inline';
	document.AdmPost.admcan.style.display = 'none';    
	document.AdmPost.adminops.disabled = false;
	astate = false;
      }
    }
    if (elem[0].firstChild != null) {
      ainfo = document.getElementById('adminfo');
      ainfo.innerHTML = elem[0].firstChild.nodeValue;
      ainfo.style.display = 'block';
    }
    elem = xml.getElementsByTagName('node');
    if (elem.length > 0) {
      var stuff = '';
      for (var i = 0; i < elem.length; i++) {
	var dt = new Date(elem[i].getAttribute('time')*1000);
	var yd = new Date(dt.getDate()-1);
	var ts;
	if (dt < yd)
	  ts = dt.toLocaleDateString() + ' ' + dt.toLocaleTimeString();
	else
	  ts = dt.toLocaleTimeString();
	var val = '';
	if (elem[i].getAttribute('btype') != 'Static Controller') {
	  var where = elem[i].getElementsByTagName('user');
	  if (where.length > 0) {
	    var j;
	    for (j = 0; j < where[0].childNodes.length; j++) {
	      if (where[0].childNodes[j].nodeType != 1)
		continue;
	      val = where[0].childNodes[j].firstChild.nodeValue;
	      if (val == 'False')
		val = 'off';
	      else if (val == 'True')
		val = 'on';
	      break;
	    }
	  }
	}
	var id = elem[i].getAttribute('id');
	stuff=stuff+'<tr id="node'+i+'" onmouseover="this.className=\'highlight\';" onmouseout="if (this.id == curnode) this.className=\'click\'; else this.className=\'normal\';" onclick="return SaveNode(this.id);" ondblClick="SaveNode(this.id); return DisplayNode();"><td>'+id+(id == nodeid ? '*' : '')+'</td><td>'+elem[i].getAttribute('btype')+'</td><td>'+elem[i].getAttribute('gtype')+'</td><td>'+elem[i].getAttribute('manufacturer')+' '+elem[i].getAttribute('product')+'</td><td>'+elem[i].getAttribute('name')+'</td><td>'+elem[i].getAttribute('location')+'</td><td>'+val+'</td.<td>'+ts+'</td></tr>';
	CreateDivs(elem, 'user', divcur, i);
	CreateDivs(elem, 'config', divcon, i);
	CreateDivs(elem, 'system', divinfo, i);
	CreateName(elem[i].getAttribute('name'),i);
	CreateLocation(elem[i].getAttribute('location'),i);
	CreateGroup(elem,i);
	CreatePoll(elem, i);
      }
      document.getElementById('tbody').innerHTML=stuff;
      if (curnode != null)
	SaveNode(curnode);
    }
    elem = xml.getElementsByTagName('log');
    if (elem != null && elem[0].getAttribute('size') > 0) {
      var ta = document.getElementById('logdata');
      ta.innerHTML = ta.innerHTML + return2br(elem[0].firstChild.nodeValue);
      ta.scrollTop = ta.scrollHeight;
    }
    polltmr = setTimeout(Poll, 750);
  }
}
function Poll()
{
  try {
    pollhttp.open("GET", 'poll.xml', true);
    pollhttp.onreadystatechange = PollReply;
    if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
      pollhttp.send(null);
    } else {// code for IE6, IE5
      pollhttp.send();
    }
    pollwait = setTimeout(PollTimeout, 3000); //3 seconds
  } catch (e) {
    pollwait = setTimeout(PollTimeout, 3000); //3 seconds
  }
}
function BED()
{
  var forms = document.forms;
  var off = (dev.length == 0) && !usb;

  for (var i = 0; i < forms.length; i++) {
    if (forms[i].name == '')
      continue;
    for (var j = 0; j < forms[i].elements.length; j++) {
      if (forms[i].elements[j].tagName == 'BUTTON' && forms[i].elements[j].name != 'initialize')
        forms[i].elements[j].disabled = off;
      else
	forms[i].elements[j].disabled = !off;
    }
  }
  document.getElementById('configcur').disabled = off;
  document.getElementById('configcur').checked = true;
  document.getElementById('configcon').disabled = off;
  document.getElementById('configcon').checked = false;
  document.getElementById('configinfo').disabled = off;
  document.getElementById('configinfo').checked = false;
  document.AdmPost.adminops.selectedIndex = 0;
  document.AdmPost.adminops.disabled = off;
  document.NodePost.nodeops.selectedIndex = 0;
  document.NodePost.nodeops.disabled = off;
  if (off) {
    document.getElementById('configcur').innerHTML = '';
    document.getElementById('configcon').innerHTML = '';
    document.getElementById('configinfo').innerHTML = '';
  } else {
    document.DevPost.devname.value = dev;
    document.DevPost.usbb.checked = usb;
  }
  if (!off) {
    Poll();
  } else {
    pollhttp.abort();
    clearTimeout(polltmr);
    clearTimeout(pollwait);
  }
}
function DoConfig(id)
{
  if (curnode != null) {
    var dcur=document.getElementById('divconfigcur');
    var dcon=document.getElementById('divconfigcon');
    var dinfo=document.getElementById('divconfiginfo');
    var node=curnode.substr(4);

    dcur.innerHTML=divcur[node];
    dcon.innerHTML=divcon[node];
    dinfo.innerHTML=divinfo[node];
    if (id == 'configcur') {
      dcur.className = '';
      dcon.className = 'hide';
      dinfo.className = 'hide';
    } else if (id == 'configcon') {
      dcon.className = '';
      dcur.className = 'hide';
      dinfo.className = 'hide';
    } else {
      dinfo.className = '';
      dcur.className = 'hide';
      dcon.className = 'hide';
    }
    return true;
  } else {
    return false;
  }
}
function DoValue(id)
{
  if (curnode != null) {
    var posthttp;
    var params;
    var arg=document.getElementById(id).value;

    if (arg.toLowerCase() == 'off')
      arg = 'false';
    else if (arg.toLowerCase() == 'on')
      arg = 'true';
    params=id+'='+arg;
    if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
      posthttp=new XMLHttpRequest();
    } else {
      posthttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST','valuepost.html',false);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.setRequestHeader("Content-length", params.length);
    posthttp.setRequestHeader("Connection", "close");
    posthttp.send(params);
  }
  return false;
}
function DoDevUSB()
{
  document.DevPost.devname.disabled = document.DevPost.usbb.checked;
  return true;
}
function DoDevPost(fun)
{
  if (document.DevPost.devname.value.length > 0 || document.DevPost.usbb.checked) {
    document.DevPost.action='/devpost.html?dev='+document.DevPost.devname.value+'&fn='+fun+'&usb='+document.DevPost.usbb.checked;
    document.DevPost.submit();
    return true;
  } else
    return false;
}
function DoAdmPost(can)
{
  var posthttp;
  var fun;
  var params;

  if (can) {
    fun = 'cancel';
    ainfo = document.getElementById('adminfo');
    ainfo.innerHTML = 'Cancelling controller function.';
    ainfo.style.display = 'block';
  } else {
    fun = document.AdmPost.adminops.value;
    if (fun == 'choice')
      return false;
  }
  params = 'fun='+fun;

  if ((fun == 'hnf' || fun == 'remfn' || fun == 'repfn' || fun == 'reqnu' || fun == 'reqnnu' ||
       fun == 'assrr' || fun == 'delarr') && curnode == null) {
    ainfo = document.getElementById('adminfo');
    ainfo.innerHTML = 'Must select a node below for this function.';
    ainfo.style.display = 'block';
    return false;
  }

  params = params+'&node='+curnode;
  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','admpost.html',false);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.setRequestHeader("Content-length", params.length);
  posthttp.setRequestHeader("Connection", "close");
  posthttp.send(params);
  return false;
}
function DoAdmHelp()
{
  ainfo = document.getElementById('adminfo');
  if (document.AdmPost.adminops.value == 'addc') {
    ainfo.innerHTML = 'Add a new secondary controller to the Z-Wave network.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'addd') {
    ainfo.innerHTML = 'Add a new device (but not a controller) to the Z-Wave network.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'cprim') {
    ainfo.innerHTML = '(Not yet implemented)';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'rconf') {
    ainfo.innerHTML = 'Receive configuration from another controller.';   
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'remc') {
    ainfo.innerHTML = 'Remove a controller from the Z-Wave network.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'remd') {
    ainfo.innerHTML = 'Remove a device (but not a controller) from the Z-Wave network.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'remfn') {
    ainfo.innerHTML = 'Move a node to the controller\'s list of failed nodes.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'hnf') {
    ainfo.innerHTML = 'Check whether a node is in the controller\'s failed nodes list.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'repfn') {
    ainfo.innerHTML = 'Replace a failed device with another.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'tranpr') {
    ainfo.innerHTML = '(Not yet implemented) - Add a new controller to the network and make it the primary.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'reqnu') {
    ainfo.style.display = 'block';
    ainfo.innerHTML = 'Update the controller with network information from the SUC/SIS.';
  } else if (document.AdmPost.adminops.value == 'reqnnu') {
    ainfo.innerHTML = 'Get a node to rebuild it\'s neighbour list.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'assrr') {
    ainfo.innerHTML = 'Assign a network return route to a device.';
    ainfo.style.display = 'block';
  } else if (document.AdmPost.adminops.value == 'delarr') {
    ainfo.innerHTML = 'Delete all network return routes from a device.';
    ainfo.style.display = 'block';
  } else {
    ainfo.style.display = 'none';
  }
  return true;
}
function DoNodePost(val)
{
  var posthttp;
  var fun;
  var params;

  fun = document.NodePost.nodeops.value;
  if (fun == 'choice')
    return false;

  params = 'fun='+fun+'&node='+curnode+'&value='+val;
  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','nodepost.html',false);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.setRequestHeader("Content-length", params.length);
  posthttp.setRequestHeader("Connection", "close");
  posthttp.send(params);
  return false;
}
function DoNodeHelp()
{
  var ninfo = document.getElementById('nodeinfo');
  if (curnode == null) {
    ninfo.innerHTML = 'Must select a node below for this function.';
    ninfo.style.display = 'block';
    document.NodePost.nodeops.selectedIndex = 0;
    return false;
  }
  var node=curnode.substr(4);
  var ncntl = document.getElementById('nodecntl');
  if (document.NodePost.nodeops.value == 'nam') {
    ninfo.innerHTML = 'Node naming functions.';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodename[node];
    ncntl.style.display = 'block';
    document.NodePost.name.select();
  } else if (document.NodePost.nodeops.value == 'loc') {
    ninfo.innerHTML = 'Location functions.';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodeloc[node];
    ncntl.style.display = 'block';
    document.NodePost.location.select();
  } else if (document.NodePost.nodeops.value == 'grp') {
    ninfo.innerHTML = 'Group/Association functions';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodegrp[node]+nodegrpgrp[node][1];
    ncntl.style.display = 'block';
  } else if (document.NodePost.nodeops.value == 'pol') {
    ninfo.innerHTML = 'Polling settings';
    ninfo.style.display = 'block';
    ncntl.innerHTML = nodepoll[node]+nodepollpoll[node][0];
    ncntl.style.display = 'block';
  } else {
    ninfo.style.display = 'none';
    ncntl.style.display = 'none';
  }
  return true;
}
function DoGroup()
{
  var node=curnode.substr(4);
  var ngrp = document.getElementById('nodegrp');

  ngrp.innerHTML = nodegrpgrp[node][document.NodePost.group.value];
  return true;
}
function DoGrpPost()
{
  var posthttp;
  var params='fun=group&node='+curnode+'&num='+document.NodePost.group.value+'&groups=';
  var opts=document.NodePost.groups.options;
  var i;

  for (i = 0; i < opts.length; i++)
    if (opts[i].selected) {
      params=params+opts[i].text+',';
    }

  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','grouppost.html',false);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.setRequestHeader("Content-length", params.length);
  posthttp.setRequestHeader("Connection", "close");
  posthttp.send(params);

  return false;
}
function DoPoll()
{
  var node=curnode.substr(4);
  var npoll = document.getElementById('nodepoll');

  npoll.innerHTML = nodepollpoll[node][document.NodePost.polled.value];
  return true;
}
function DoPollPost()
{
  var posthttp;
  var params='fun=poll&node='+curnode+'&ids=';
  var opts=document.NodePost.polls.options;
  var i;

  for (i = 0; i < opts.length; i++)
    params=params+opts[i].id+',';
  params=params+'&poll=';
  for (i = 0; i < opts.length; i++)
    if (opts[i].selected)
      params=params+'1,';
    else
      params=params+'0,';
	

  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','pollpost.html',false);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.setRequestHeader("Content-length", params.length);
  posthttp.setRequestHeader("Connection", "close");
  posthttp.send(params);

  return false;
}
function DoSavePost()
{
  var posthttp;
  var params='fun=save';

  if (window.XMLHttpRequest) {// code for IE7+, Firefox, Chrome, Opera, Safari
    posthttp=new XMLHttpRequest();
  } else {
    posthttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  posthttp.open('POST','savepost.html',false);
  posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  posthttp.setRequestHeader("Content-length", params.length);
  posthttp.setRequestHeader("Connection", "close");
  posthttp.send(params);

  return false;
}
function return2br(dataStr) {
  return dataStr.replace(/(\r\n|[\r\n])/g, "<br />");
}
function boxsize(field)
{
  if (field.length < 8)
    return 8;
  return field.length+2;
}
function CreateOnOff(label,value,units,id,ro)
{
  var data='<tr><td style="float: right;"><label><span class="legend">'+label+':&nbsp;</span></label></td><td><select id="'+id+'" onchange="return DoValue(\''+id+'\');"'
  if (ro)
    data=data+' disabled="true"';
  data=data+'>';
  if (value == 'True')
    data=data+'<option value="off">Off</option><option value="on" selected="true">On</option>';
  else
    data=data+'<option value="off" selected="true">Off</option><option value="on">On</option>';
  data=data+'</select></td><td><span class="legend">'+units+'</span></td></tr>';
  return data;
}
function CreateTextBox(label,value,units,id,ro)
{
  var data = '<tr><td style="float: right;"><label><span class="legend">'+label+':&nbsp;</span></label></td><td><input type="text" class="legend" size="'+boxsize(value)+'" id="'+id+'" value="'+value+'"';
  if (ro)
    data=data+' disabled="true">';
  else
    data=data+'>';
  data=data+'<span class="legend">'+units+'</span>';
  if (!ro)
    data=data+'<button type="submit" onclick="return DoValue(\''+id+'\');">Submit</button></td></tr>';
  return data;
}
function CreateList(label,value,units,id,options,ro)
{
  var cnt=options.length;
  var i;
  var data='<tr><td style="float: right;"><label><span class="legend">'+label+':&nbsp;</span></label></td><td><select id="'+id+'" onchange="return DoValue(\''+id+'\');"';
  if (ro)
    data=data+' disabled="true">';
  else
    data=data+'>';
  for (i=0; i<cnt; i++) {
    var opt=options[i].firstChild.nodeValue;
    data=data+'<option value="'+opt+'"';
    if (opt == value)
      data=data+' selected="true"';
    data=data+'>'+opt+'</option>';
  }
  data=data+'</select><span class="legend">'+units+'</span></td></tr>';
  return data;
}
function CreateLabel(label,value,units,id)
{
    return '<tr><td style="float: right;"><label><span class="legend">'+label+':&nbsp;</span></label></td><td><input type="text" class="legend" disabled="true" size="'+boxsize(value)+'" id="'+id+'" value="'+value+'"><span class="legend">'+units+'</span></td></tr>';
}
function CreateDivs(elem,genre,divto,node)
{
  var where=elem[node].getElementsByTagName(genre);

  divto[node]='<table border="0" cellpadding="1" cellspacing="0"><tbody>';
  if (where.length > 0) {
    var i;
    var lastclass='';
    for (i = 0; i < where[0].childNodes.length; i++) {
      if (where[0].childNodes[i].nodeType != 1)
	continue;
      var ro=where[0].childNodes[i].getAttribute('readonly')=='true';
      var cls=where[0].childNodes[i].getAttribute('class');
      var tag=where[0].childNodes[i].tagName;
      var id=(node+1)+'-'+cls+'-'+genre+'-'+tag+'-'+where[0].childNodes[i].getAttribute('instance')+'-'+where[0].childNodes[i].getAttribute('index');
      if (tag == 'bool') {
	  divto[node]=divto[node]+CreateOnOff(where[0].childNodes[i].getAttribute('label'),where[0].childNodes[i].firstChild.nodeValue,where[0].childNodes[i].getAttribute('units'),id,ro);
      } else if (tag == 'byte') {
        if (lastclass == 'BASIC' && cls == 'SWITCH MULTILEVEL')
          divto[node]='';
        lastclass=cls;
        divto[node]=divto[node]+CreateTextBox(where[0].childNodes[i].getAttribute('label'),where[0].childNodes[i].firstChild.nodeValue,where[0].childNodes[i].getAttribute('units'),id,ro);
      } else if (tag == 'int') {
	  divto[node]=divto[node]+CreateTextBox(where[0].childNodes[i].getAttribute('label'),where[0].childNodes[i].firstChild.nodeValue,where[0].childNodes[i].getAttribute('units'),id,ro);
      } else if (tag == 'short') {
	  divto[node]=divto[node]+CreateTextBox(where[0].childNodes[i].getAttribute('label'),where[0].childNodes[i].firstChild.nodeValue,where[0].childNodes[i].getAttribute('units'),id,ro);
      } else if (tag == 'list') {
	  divto[node]=divto[node]+CreateList(where[0].childNodes[i].getAttribute('label'),where[0].childNodes[i].getAttribute('current'),where[0].childNodes[i].getAttribute('units'),id,where[0].childNodes[i].getElementsByTagName('item'),ro);
      } else if (tag == 'string') {
	  divto[node]=divto[node]+CreateLabel(where[0].childNodes[i].getAttribute('label'),where[0].childNodes[i].firstChild.nodeValue,where[0].childNodes[i].getAttribute('units'),id);
      } else {
      }
    }
  }
  divto[node]=divto[node]+'</tbody></table>';
}
function CreateName(val,node)
{
  nodename[node]='<tr><td style="float: right;"><label><span class="legend">Name:&nbsp;</span></label></td><td><input type="text" class="legend" size="'+boxsize(val)+'" id="name" value="'+val+'"><button type="submit" style="margin-left: 5px;" onclick="return DoNodePost(document.NodePost.name.value);">Submit</button></td></tr>';
}
function CreateLocation(val,node)
{
  nodeloc[node]='<tr><td style="float: right;"><label><span class="legend">Location:&nbsp;</span></label></td><td><input type="text" class="legend" size="'+boxsize(val)+'" id="location" value="'+val+'"><button type="submit" style="margin-left: 5px;" onclick="return DoNodePost(document.NodePost.location.value);">Submit</button></td></tr>';
}
function CreateGroup(elem,node)
{
  var where=elem[node].getElementsByTagName('groups');
  var cnt=where[0].getAttribute('cnt');
  var grp;
  var i, j, k;
  var id;
  var mx;
  var str;

  if (cnt == 0) {
    nodegrp[node]='';
    nodegrpgrp[node] = new Array();
    nodegrpgrp[node][1]='';
    return;
  }
  nodegrp[node]='<tr><td style="float: right;"><label><span class="legend">Groups:&nbsp;</span></label></td><td><select id="group" style="margin-left: 5px;" onchange="return DoGroup();">';
  nodegrpgrp[node] = new Array(cnt);
  grp = 1;
  for (i = 0; i < where[0].childNodes.length; i++) {
    if (where[0].childNodes[i].nodeType != 1)
      continue;
    id = where[0].childNodes[i].getAttribute('ind');
    nodegrp[node]=nodegrp[node]+'<option value="'+id+'">'+where[0].childNodes[i].getAttribute('label')+' ('+id+')</option>';
    mx = where[0].childNodes[i].getAttribute('max');
    if (where[0].childNodes[i].firstChild != null) {
      str = where[0].childNodes[i].firstChild.nodeValue;
      str = str.split(",");
    } else
      str = new Array();
    nodegrpgrp[node][grp] = '<td><div id="nodegrp" name="nodegrp" style="float: right;"><select id="groups" multiple size="4" style="vertical-align: top; margin-left: 5px;">';
    k = 0;
    for (j = 1; j <= nodecount; j++) {
      while (k < str.length && str[k] < j)
	k++;
      if (str[k] == j)
	nodegrpgrp[node][grp]=nodegrpgrp[node][grp]+'<option selected="true">'+j+'</option>';
      else
	nodegrpgrp[node][grp]=nodegrpgrp[node][grp]+'<option>'+j+'</option>';
    }
    nodegrpgrp[node][grp]=nodegrpgrp[node][grp]+'</select></td><td><button type="submit" style="margin-left: 5px;" onclick="return DoGrpPost();">Submit</button></div></td></tr>';
    grp++;
  }
  nodegrp[node]=nodegrp[node]+'</select></td>';
}
function CreatePoll(elem,node)
{
  if (elem[node].getElementsByTagName('user').length > 0 || elem[node].getElementsByTagName('system').length > 0)
    nodepoll[node]='<tr><td style="float: right;"><label><span class="legend">Polling&nbsp;</span></label></td><td><select id="polled" style="margin-left: 5px;" onchange="return DoPoll();"><option value="0">User</option><option value="1">System</option></select></td>';
  else
    nodepoll[node]='';
  nodepollpoll[node] = new Array(2);
  CreatePollPoll(elem,'user',node);
  CreatePollPoll(elem,'system',node);
}
function CreatePollPoll(elem,genre,node)
{
  var ind;
  var where=elem[node].getElementsByTagName(genre);
  if (genre == 'user')
    ind = 0;
  else
    ind = 1;
  nodepollpoll[node][ind]='<td><div id="nodepoll" name="nodepoll" style="float: right;"><select id="polls" multiple size="4" style="vertical-align: top; margin-left: 5px;">';
  if (where.length > 0) {
    var i;
    for (i = 0; i < where[0].childNodes.length; i++) {
      if (where[0].childNodes[i].nodeType != 1)
	continue;
      var p=where[0].childNodes[i].getAttribute('polled') == 'true';
      var id=(node+1)+'-'+where[0].childNodes[i].getAttribute('class')+'-'+genre+'-'+where[0].childNodes[i].tagName+'-'+where[0].childNodes[i].getAttribute('instance')+'-'+where[0].childNodes[i].getAttribute('index');
      nodepollpoll[node][ind]=nodepollpoll[node][ind]+'<option id="'+id+'"';
      if (p)
	  nodepollpoll[node][ind]=nodepollpoll[node][ind]+' selected="true"';
      nodepollpoll[node][ind]=nodepollpoll[node][ind]+'>'+where[0].childNodes[i].getAttribute('label')+'</option>';
    }
    nodepollpoll[node][ind]=nodepollpoll[node][ind]+'</select></td><button type="submit" style="margin-left: 5px;" onclick="return DoPollPost();">Submit</button></div></td></tr>';
  } else
    nodepollpoll[node][ind]='';
}
