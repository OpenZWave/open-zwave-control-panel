var pollhttp;
var topohttp;
var stathttp;
var atsthttp;
var racphttp;
var polltmr = null;
var pollwait = null;
var divcur = new Array();
var divcon = new Array();
var divinfo = new Array();
var nodes = new Array();
var nodename = new Array();
var nodeloc = new Array();
var nodegrp = new Array();
var nodegrpgrp = new Array();
var nodepoll = new Array();
var nodepollpoll = new Array();
var astate = false;
var nodecount;
var nodeid;
var sucnodeid;
var tt_top = 3;
var tt_left = 3;
var tt_maxw = 300;
var tt_speed = 10;
var tt_timer = 20;
var tt_endalpha = 95;
var tt_alpha = 0;
var tt_h = 0;
var tt = document.createElement('div');
var t = document.createElement('div');
var c = document.createElement('div');
var b = document.createElement('div');
var ie = document.all ? true : false;
var curnode = null;
var routes = new Array();
var curclassstat = null;
var classstats = new Array();
if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
    pollhttp = new XMLHttpRequest();
} else {
    pollhttp = new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
    topohttp = new XMLHttpRequest();
} else {
    topohttp = new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
    stathttp = new XMLHttpRequest();
} else {
    stathttp = new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
    atsthttp = new XMLHttpRequest();
} else {
    atsthttp = new ActiveXObject("Microsoft.XMLHTTP");
}
if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
    racphttp = new XMLHttpRequest();
} else {
    racphttp = new ActiveXObject("Microsoft.XMLHTTP");
}

function GetDefaultDevice() {
    var devhttp;
    if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
        devhttp = new XMLHttpRequest();
    } else {
        devhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    devhttp.onreadystatechange = function () {
        if (devhttp.readyState == 4 && devhttp.status == 200) {
            if (devhttp.responseText == 'NULL')
                document.DevPost.devname.value = '';
            else
                document.DevPost.devname.value = devhttp.responseText;
        }
        BED();
    }
    devhttp.open("GET", "currdev", true);
    if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
        devhttp.send(null);
    } else { // code for IE6, IE5
        devhttp.send();
    }
}

function OptionGroup(label, disabled) {
    var element = document.createElement('optgroup');
    if (disabled !== undefined) element.disabled = disabled;
    if (label !== undefined) element.label = label;
    return element;
}

function SaveNode(newid) {
    var i = newid.substr(4);
    var c = -1;
    if (curnode != null)
        c = curnode.substr(4);
    document.getElementById('divconfigcur').innerHTML = divcur[i];
    document.getElementById('divconfigcon').innerHTML = divcon[i];
    document.getElementById('divconfiginfo').innerHTML = divinfo[i];
    if (c != -1) {
        if (i != c)
            document.getElementById(curnode).className = 'normal';
    } else {
        document.getElementById('configcur').disabled = false;
        document.getElementById('configcon').disabled = false;
        document.getElementById('configinfo').disabled = false;
    }
    curnode = newid;
    DoNodeHelp();
    $('#devices tr.success').removeClass('success');
    $('#' + curnode).addClass('success');
    return true;
}

function ClearNode() {
    if (curnode != null) {
        document.getElementById(curnode).className = 'normal';
        document.NodePost.nodeops.selectedIndex = 0;
        document.getElementById('divconfigcur').innerHTML = '';
        document.getElementById('divconfigcon').innerHTML = '';
        document.getElementById('divconfiginfo').innerHTML = '';
        document.getElementById('nodeinfo').style.display = 'none';
        document.getElementById('nodecntl').style.display = 'none';
        curnode = null;
    }
    return true;
}

function DisplayNode(n) {
    return true;
}

function PollTimeout() {
    pollhttp.abort();
    Poll();
}

function Poll() {
    try {
        pollhttp.open("GET", 'poll.xml', true);
        pollhttp.onreadystatechange = PollReply;
        if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
            pollhttp.send(null);
        } else { // code for IE6, IE5
            pollhttp.send();
        }
        pollwait = setTimeout(PollTimeout, 3000); //3 seconds
    } catch (e) {
        pollwait = setTimeout(PollTimeout, 3000); //3 seconds
    }
}


function PollReply() {
    var xml;
    if (pollhttp.readyState == 4 && pollhttp.status == 200) {
        clearTimeout(pollwait);
        xml = pollhttp.responseXML;
        var poll_elems = xml.getElementsByTagName('poll');
        if (poll_elems.length > 0) {
            var changed = false;
            var poll_elem = poll_elems[0];
            if (poll_elem.getAttribute('homeid') != document.getElementById('homeid').value)
                document.getElementById('homeid').value = poll_elem.getAttribute('homeid');
            if (poll_elem.getAttribute('nodecount') != nodecount) {
                nodecount = poll_elem.getAttribute('nodecount');
                document.getElementById('nodecount').value = nodecount;
            }
            if (poll_elem.getAttribute('nodeid') != nodeid) {
                nodeid = poll_elem.getAttribute('nodeid');
            }
            if (poll_elem.getAttribute('sucnodeid') != sucnodeid) {
                sucnodeid = poll_elem.getAttribute('sucnodeid');
                document.getElementById('sucnodeid').value = sucnodeid;
            }
            if (poll_elem.getAttribute('cmode') != document.getElementById('cmode').value)
                document.getElementById('cmode').value = poll_elem.getAttribute('cmode');
            if (poll_elem.getAttribute('noop') == '1') {
                var testhealreport = document.getElementById('testhealreport');
                testhealreport.innerHTML = testhealreport.innerHTML + 'No Operation message completed.<br>';
            }
            var admin_elem = xml.getElementsByTagName('admin');
            var admin_elem = admin_elem[0];
            if (admin_elem.getAttribute('active') == 'true') {
                if (!astate) {
                    document.AdmPost.admgo.style.display = 'none';
                    document.AdmPost.admcan.style.display = 'inline';
                    document.AdmPost.adminops.disabled = true;
                    astate = true;
                }
            } else if (admin_elem.getAttribute('active') == 'false') {
                if (astate) {
                    document.AdmPost.admgo.style.display = 'inline';
                    document.AdmPost.admcan.style.display = 'none';
                    document.AdmPost.adminops.disabled = false;
                    astate = false;
                }
            }
            if (admin_elem.firstChild != null) {
                ainfo = document.getElementById('adminfo');
                ainfo.innerHTML = admin_elem.firstChild.nodeValue;
                ainfo.style.display = 'block';
            }
            var update_elem = xml.getElementsByTagName('update');
            if (update_elem.length > 0) {
                var remove = update_elem[0].getAttribute('remove');
                if (remove != undefined) {
                    var remnodes = remove.split(',');
                    changed = true;
                    for (var i = 0; i < remnodes.length; i++) {
                        nodes[remnodes[i]] = null;
                        if (curnode == ('node' + remnodes[i]))
                            ClearNode();
                    }
                }
            }
            var node_elems = xml.getElementsByTagName('node');
            changed |= node_elems.length > 0;
            for (var i = 0; i < node_elems.length; i++) {
                var node_elem = node_elems[i];
                var id = node_elem.getAttribute('id');
                nodes[id] = {
                    time: node_elem.getAttribute('time'),
                    btype: node_elem.getAttribute('btype'),
                    id: node_elem.getAttribute('id'),
                    gtype: node_elem.getAttribute('gtype'),
                    manufacturer: node_elem.getAttribute('manufacturer'),
                    product: node_elem.getAttribute('product'),
                    name: node_elem.getAttribute('name'),
                    location: node_elem.getAttribute('location'),
                    listening: node_elem.getAttribute('listening') == 'true',
                    frequent: node_elem.getAttribute('frequent') == 'true',
                    zwaveplus: node_elem.getAttribute('zwaveplus') == 'true',
                    beam: node_elem.getAttribute('beam') == 'true',
                    routing: node_elem.getAttribute('routing') == 'true',
                    security: node_elem.getAttribute('security') == 'true',
                    status: node_elem.getAttribute('status'),
                    values: null,
                    groups: null
                };
                var k = 0;
                var values_node = node_elem.getElementsByTagName('value');
                var id_node = nodes[id];
                id_node.values = new Array();
                for (var j = 0; j < values_node.length; j++) {
                    var values = values_node[j];
                    id_node.values[k] = {
                        readonly: values.getAttribute('readonly') == 'true',
                        genre: values.getAttribute('genre'),
                        cclass: values.getAttribute('class'),
                        type: values.getAttribute('type'),
                        instance: values.getAttribute('instance'),
                        index: values.getAttribute('index'),
                        label: values.getAttribute('label'),
                        units: values.getAttribute('units'),
                        polled: values.getAttribute('polled') == true,
                        help: null,
                        value: null
                    };
                    var help = values.getElementsByTagName('help');
                    var node_values = id_node.values[k];
                    if (help.length > 0)
                        node_values.help = help[0].firstChild.nodeValue;
                    else
                        node_values.help = '';
                    if (node_values.type == 'list') {
                        var items = values.getElementsByTagName('item');
                        var current = values.getAttribute('current');
                        node_values.value = new Array();
                        for (var l = 0; l < items.length; l++) {
                            if (items[l].firstChild) {
                                node_values.value[l] = {
                                    item: items[l].firstChild.nodeValue,
                                    selected: (current == items[l].firstChild.nodeValue)
                                };
                            } else {
                                node_values.value[l] = {
                                    item: "---",
                                    selected: false
                                };
                            }
                        }
                    } else if (node_values.type == 'bitset') {
                        var bits = values.getElementsByTagName('bitset');
                        alert("BitSet ValueID Not Implemented in JavaScript. Please Help us out if you can!");
                        //                   	for (var l = 0; l < bits.length; l++) {
                        //                   		alert(bits[l].getAttribute('label');
                        //                   	}
                    } else {
                        var val = values.getAttribute('val');
                        if (val != null)
                            node_values.value = val;
                        else
                            node_values.value = '---';
                    }
                    k++;
                }
                var groups = node_elem.getElementsByTagName('groups');
                id_node.groups = new Array();
                groups = groups[0].getElementsByTagName('group');
                k = 0;
                for (var j = 0; j < groups.length; j++) {
                    var group = groups[j];
                    id_node.groups[k] = {
                        id: group.getAttribute('ind'),
                        max: group.getAttribute('max'),
                        label: group.getAttribute('label'),
                        nodes: null
                    };
                    if (group.firstChild != null)
                        id_node.groups[k].nodes = group.firstChild.nodeValue.split(',');
                    else
                        id_node.groups[k].nodes = new Array();
                    k++;
                }
            }
            var log_elems = xml.getElementsByTagName('log');
            var log_elem = log_elems[0];
            if (log_elems != null && log_elem.getAttribute('size') > 0) {
                var ta = document.getElementById('logdata');
                ta.innerHTML = ta.innerHTML + return2br(log_elem.firstChild.nodeValue);
                ta.scrollTop = ta.scrollHeight;
            }
            if (changed) {
                var stuff = '';
                for (var i = 1; i < nodes.length; i++) {
                    var node = nodes[i];
                    if (node == null)
                        continue;
                    var dt = new Date(node.time * 1000);
                    var yd = new Date(dt.getDate() - 1);
                    var ts;
                    var ext;
                    var exthelp;
                    if (dt < yd)
                        ts = dt.toLocaleDateString() + ' ' + dt.toLocaleTimeString();
                    else
                        ts = dt.toLocaleTimeString();
                    var val = '';
                    if (node.values.length > 0)
                        for (var j = 0; j < node.values.length; j++) {
                            if (node.values[j].genre != 'user')
                                continue;
                            if (node.values[j].type == 'list') {
                                for (var l = 0; l < node.values[j].value.length; l++)
                                    if (!node.values[j].value[l].selected)
                                        continue;
                                    else
                                        val = node.values[j].value[l].item;
                            } else if (node.values[j] != null) {
                                val = node.values[j].value;
                                if (val == 'False')
                                    val = 'off';
                                else if (val == 'True')
                                    val = 'on';
                            }
                            break;
                        }
                    ext = ' ';
                    exthelp = '';
                    if (node.id == nodeid) {
                        ext += '*';
                        exthelp += 'controller, ';
                    }
                    if (node.listening) {
                        ext += 'L';
                        exthelp += 'listening, ';
                    }
                    if (node.frequent) {
                        ext += 'F';
                        exthelp += 'FLiRS, ';
                    }
                    if (node.beam) {
                        ext += 'B';
                        exthelp += 'beaming, ';
                    }
                    if (node.routing) {
                        ext += 'R';
                        exthelp += 'routing, ';
                    }
                    if (node.security) {
                        ext += 'S';
                        exthelp += 'security, ';
                    }
                    if (node.zwaveplus) {
                        ext += "+";
                        exthelp += 'ZwavePlus, ';
                    }
                    if (exthelp.length > 0)
                        exthelp = exthelp.substr(0, exthelp.length - 2);
                    stuff += '<tr id="node' + i + '"onclick="return SaveNode(this.id);" ondblClick="ClearNode(); return DisplayNode();"><td onmouseover="ShowToolTip(\'' + exthelp + '\',0);" onmouseout="HideToolTip();">' + node.id + ext + '</td><td>' + node.btype + '</td><td>' + node.gtype + '</td><td>' + node.manufacturer + ' ' + node.product + '</td><td>' + node.name + '</td><td>' + node.location + '</td><td>' + val + '</td><td>' + ts + '</td><td>' + node.status + '</td></tr>';
                    CreateDivs('user', divcur, i);
                    CreateDivs('config', divcon, i);
                    CreateDivs('system', divinfo, i);
                    CreateName(node.name, i);
                    CreateLocation(node.location, i);
                    CreateGroup(i);
                    CreatePoll(i);
                }
                document.getElementById('tbody').innerHTML = stuff;
                if (curnode != null)
                    SaveNode(curnode);
            }
        }
        polltmr = setTimeout(Poll, 3000);
    }
}

function BED() {
    var forms = document.forms;
    var off = (document.DevPost.devname.value.length == 0) && !document.DevPost.usbb.checked;
    var info;

    tt.setAttribute('id', 'tt');
    t.setAttribute('id', 'tttop');
    c.setAttribute('id', 'ttcont');
    b.setAttribute('id', 'ttbot');
    tt.appendChild(t);
    tt.appendChild(c);
    tt.appendChild(b);
    document.body.appendChild(tt);
    tt.style.opacity = 0;
    tt.style.filter = 'alpha(opacity=0)';
    tt.style.display = 'none';
    for (var i = 0; i < forms.length; i++) {
        var form = forms[i];
        if (form.name == '')
            continue;
        for (var j = 0; j < form.elements.length; j++) {
            var element = form.elements[j];
            if ((element.name == 'initialize') ||
                (element.name == 'devname') ||
                (element.name == 'usbb'))
                continue;
            if ((element.tagName == 'BUTTON') ||
                (element.tagName == 'SELECT') ||
                (element.tagName == 'INPUT'))
                element.disabled = off;
            else
                element.disabled = !off;
        }
    }
    document.getElementById('configcur').disabled = off;
    document.getElementById('configcur').checked = true;
    document.getElementById('configcon').disabled = off;
    document.getElementById('configcon').checked = false;
    document.getElementById('configinfo').disabled = off;
    document.getElementById('configinfo').checked = false;
    // document.NetPost.netops.selectedIndex = 0;
    // document.NetPost.netops.disabled = off;
    info = document.getElementById('netinfo');
    info.style.display = 'none';
    document.AdmPost.adminops.selectedIndex = 0;
    document.AdmPost.adminops.disabled = off;
    info = document.getElementById('adminfo');
    info.style.display = 'none';
    info = document.getElementById('admcntl');
    info.style.display = 'none';
    document.NodePost.nodeops.selectedIndex = 0;
    document.NodePost.nodeops.disabled = off;
    info = document.getElementById('nodeinfo');
    info.style.display = 'none';
    info = document.getElementById('nodecntl');
    info.style.display = 'none';
    if (off) {
        document.getElementById('homeid').value = '';
        document.getElementById('cmode').value = '';
        document.getElementById('nodecount').value = '';
        document.getElementById('sucnodeid').value = '';
        document.getElementById('tbody').innerHTML = '';
        document.getElementById('divconfigcur').innerHTML = '';
        document.getElementById('divconfigcon').innerHTML = '';
        document.getElementById('divconfiginfo').innerHTML = '';
        document.getElementById('logdata').innerHTML = '';
    }
    if (!off) {
        Poll();
    } else {
        pollhttp.abort();
        clearTimeout(polltmr);
        clearTimeout(pollwait);
    }
    curnode = null;
}

function ShowToolTip(help, width) {
    tt.style.display = 'block';
    c.innerHTML = help;
    tt.style.width = width ? width + 'px' : 'auto';
    if (!width && ie) {
        t.style.display = 'none';
        b.style.display = 'none';
        tt.style.width = tt.offsetWidth;
        t.style.display = 'block';
        b.style.display = 'block';
    }
    if (tt.offsetWidth > tt_maxw) {
        tt.style.width = tt_maxw + 'px';
    }
    tt_h = parseInt(tt.offsetHeight) + tt_top;
    clearInterval(tt.timer);
    tt.timer = setInterval(function () {
        FadeToolTip(1);
    }, tt_timer)
}

function PosToolTip(e) {
    tt.style.top = ((ie ? event.clientY + document.documentElement.scrollTop : e.pageY) - tt_h) + 'px';
    tt.style.left = ((ie ? event.clientX + document.documentElement.scrollLeft : e.pageX) + tt_left) + 'px';
}

function FadeToolTip(d) {
    var a = tt_alpha;
    if ((a != tt_endalpha && d == 1) || (a != 0 && d == -1)) {
        var i = tt_speed;
        if (tt_endalpha - a < tt_speed && d == 1) {
            i = tt_endalpha - a;
        } else if (tt_alpha < tt_speed && d == -1) {
            i = a;
        }
        tt_alpha = a + (i * d);
        tt.style.opacity = tt_alpha * .01;
        tt.style.filter = 'alpha(opacity=' + tt_alpha + ')';
    } else {
        clearInterval(tt.timer);
        if (d == -1) {
            tt.style.display = 'none';
        }
    }
}

function HideToolTip() {
    clearInterval(tt.timer);
    tt.timer = setInterval(function () {
        FadeToolTip(-1);
    }, tt_timer);
}

function DoConfig(id) {
    if (curnode != null) {
        var dcur = document.getElementById('divconfigcur');
        var dcon = document.getElementById('divconfigcon');
        var dinfo = document.getElementById('divconfiginfo');
        var node = curnode.substr(4);

        dcur.innerHTML = divcur[node];
        dcon.innerHTML = divcon[node];
        dinfo.innerHTML = divinfo[node];
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

function DoValue(id, convert) {
    if (curnode != null) {
        var posthttp;
        var params;
        var arg = document.getElementById(id).value;

        if (typeof convert == 'undefined') {
            convert = true;
        }

        if (convert) {
            if (arg.toLowerCase() == 'off')
                arg = 'false';
            else if (arg.toLowerCase() == 'on')
                arg = 'true';
        }
        params = id + '=' + arg;
        if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
            posthttp = new XMLHttpRequest();
        } else {
            posthttp = new ActiveXObject("Microsoft.XMLHTTP");
        }
        posthttp.open('POST', 'valuepost.html', true);
        posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        posthttp.send(params);
    }
    return false;
}

function DoButton(id, pushed) {
    if (curnode != null) {
        var posthttp;
        var params;
        var arg = document.getElementById(id).value;

        if (pushed)
            arg = 'true';
        else
            arg = 'false';
        params = id + '=' + arg;
        if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
            posthttp = new XMLHttpRequest();
        } else {
            posthttp = new ActiveXObject("Microsoft.XMLHTTP");
        }
        posthttp.open('POST', 'buttonpost.html', true);
        posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        posthttp.send(params);
    }
    return false;
}

function DoDevUSB() {
    document.DevPost.devname.disabled = document.DevPost.usbb.checked;
    return true;
}

function DoDevPost(fun) {
    if (document.DevPost.devname.value.length > 0 || document.DevPost.usbb.checked) {
        var posthttp;
        var params;

        params = 'dev=' + document.DevPost.devname.value + '&fn=' + fun + '&usb=' + document.DevPost.usbb.checked;
        if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
            posthttp = new XMLHttpRequest();
        } else {
            posthttp = new ActiveXObject("Microsoft.XMLHTTP");
        }
        posthttp.open('POST', 'devpost.html', true);
        posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        posthttp.send(params);
        if (fun == 'close') {
            document.DevPost.devname = '';
            document.DevPost.usbb.checked = false;
        }
        BED();
    }
    return false;
}

function DoNetHelp() {
    var ninfo = document.getElementById('netinfo');
    var topocntl = document.getElementById('topocntl');
    var topo = document.getElementById('topo');
    var statcntl = document.getElementById('statcntl');
    var statnet = document.getElementById('statnet');
    var statnode = document.getElementById('statnode');
    var statclass = document.getElementById('statclass');
    var thcntl = document.getElementById('thcntl');
    var testhealreport = document.getElementById('testhealreport');
    if (document.NetPost.netops.value == 'topo') {
        ninfo.innerHTML = 'Topology views';
        ninfo.style.display = 'block';
        topocntl.style.display = 'block';
        topo.style.display = 'block';
        statcntl.style.display = 'none';
        statnet.style.display = 'none';
        statnode.style.display = 'none';
        statclass.style.display = 'none';
        thcntl.style.display = 'none';
        testhealreport.style.display = 'none';
        TopoLoad('load');
    } else if (document.NetPost.netops.value == 'stat') {
        ninfo.innerHTML = 'Statistic views';
        ninfo.style.display = 'block';
        topocntl.style.display = 'none';
        topo.style.display = 'none';
        statcntl.style.display = 'block';
        statnet.style.display = 'block';
        statnode.style.display = 'block';
        thcntl.style.display = 'none';
        testhealreport.style.display = 'none';
        StatLoad('load');
    } else if (document.NetPost.netops.value == 'test') {
        ninfo.innerHTML = 'Test & Heal Network';
        ninfo.style.display = 'block';
        topocntl.style.display = 'none';
        topo.style.display = 'none';
        statcntl.style.display = 'none';
        statnet.style.display = 'none';
        statnode.style.display = 'none';
        statclass.style.display = 'none';
        thcntl.style.display = 'block';
        testhealreport.style.display = 'block';
    } else {
        ninfo.style.display = 'none';
        topocntl.style.display = 'none';
        topo.style.display = 'none';
        statcntl.style.display = 'none';
        statnet.style.display = 'none';
        statnode.style.display = 'none';
        statclass.style.display = 'none';
        thcntl.style.display = 'none';
        testhealreport.style.display = 'none';
    }
    return true;
}

function DoAdmPost(can) {
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
    params = 'fun=' + fun;

    if (fun == 'hnf' || fun == 'remfn' || fun == 'repfn' || fun == 'reqnu' ||
        fun == 'reqnnu' || fun == 'assrr' || fun == 'delarr' || fun == 'reps' ||
        fun == 'addbtn' || fun == 'delbtn' || fun == 'refreshnode') {
        if (curnode == null) {
            ainfo = document.getElementById('adminfo');
            ainfo.innerHTML = 'Must select a node below for this function.';
            ainfo.style.display = 'block';
            return false;
        }
        params += '&node=' + curnode;
    } else if (fun == 'snif') {
        if (curnode == null)
            params += '&node=node255';
        else
            params += '&node=' + curnode;
    }

    if (fun == 'addbtn' || fun == 'delbtn') {
        if (document.AdmPost.button.value.length == 0) {
            ainfo = document.getElementById('adminfo');
            ainfo.innerHTML = 'Button number is required.';
            ainfo.style.display = 'block';
            document.AdmPost.button.select();
            return false;
        }
        params += '&button=' + document.AdmPost.button.value;
    }

    if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
        posthttp = new XMLHttpRequest();
    } else {
        posthttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST', 'admpost.html', true);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.send(params);
    if (fun == 'remc' || fun == 'remd') {
        document.getElementById('divconfigcur').innerHTML = '';
        document.getElementById('divconfigcon').innerHTML = '';
        document.getElementById('divconfiginfo').innerHTML = '';
        curnode = null;
    }
    return false;
}

function DoAdmHelp() {
    ainfo = document.getElementById('adminfo');
    var acntl = document.getElementById('admcntl');
    acntl.innerHTML = '';
    document.AdmPost.admgo.style.display = 'inline';
    if (document.AdmPost.adminops.value == 'addd') {
        ainfo.innerHTML = 'Add a new device or controller to the Z-Wave network.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'addds') {
        ainfo.innerHTML = 'Add a new device or controller to the Z-Wave Network (Secure Option).';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'cprim') {
        ainfo.innerHTML = 'Create new primary controller in place of dead old controller.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'rconf') {
        ainfo.innerHTML = 'Receive configuration from another controller.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'remd') {
        ainfo.innerHTML = 'Remove a device or controller from the Z-Wave network.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'remfn') {
        ainfo.innerHTML = 'Remove a failed node that is on the controller\'s list of failed nodes.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'hnf') {
        ainfo.innerHTML = 'Check whether a node is in the controller\'s failed nodes list.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'repfn') {
        ainfo.innerHTML = 'Replace a failed device with a working device.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'tranpr') {
        ainfo.innerHTML = 'Transfer primary to a new controller and make current secondary.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'reqnu') {
        ainfo.style.display = 'block';
        ainfo.innerHTML = 'Update the controller with network information from the SUC/SIS.';
    } else if (document.AdmPost.adminops.value == 'reqnnu') {
        ainfo.innerHTML = 'Get a node to rebuild its neighbor list.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'assrr') {
        ainfo.innerHTML = 'Assign a network return route to a device.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'delarr') {
        ainfo.innerHTML = 'Delete all network return routes from a device.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'snif') {
        ainfo.innerHTML = 'Send a node information frame.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'reps') {
        ainfo.innerHTML = 'Send information from primary to secondary.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'addbtn' ||
        document.AdmPost.adminops.value == 'delbtn') {
        if (curnode == null) {
            ainfo.innerHTML = 'Must select a node below for this function.';
            ainfo.style.display = 'block';
            document.AdmPost.adminops.selectedIndex = 0;
            document.AdmPost.admgo.style.display = 'none';
            return false;
        }
        acntl.innerHTML = '<label style="margin-left: 10px;"><span class="legend">Button number:&nbsp;</span></label><input type="text" class="legend form-control" size="3" id="button" value="">';
        acntl.style.display = 'block';
        document.AdmPost.button.select();
        if (document.AdmPost.adminops.value == 'addbtn')
            ainfo.innerHTML = 'Add a button from a handheld.';
        else
            ainfo.innerHTML = 'Remove a button from a handheld.';
        ainfo.style.display = 'block';
    } else if (document.AdmPost.adminops.value == 'refreshnode') {
        ainfo.innerHTML = 'Refresh Node Info';
        ainfo.style.display = 'block';
    } else {
        ainfo.style.display = 'none';
        document.AdmPost.admgo.style.display = 'none';
    }
    return true;
}

function DoNodePost(val) {
    var posthttp;
    var fun;
    var params;

    fun = document.NodePost.nodeops.value;
    if (fun == 'choice')
        return false;

    params = 'fun=' + fun + '&node=' + curnode + '&value=' + val;
    if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
        posthttp = new XMLHttpRequest();
    } else {
        posthttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST', 'nodepost.html', true);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.send(params);
    return false;
}

function DoNodeHelp() {
    var ninfo = document.getElementById('nodeinfo');
    if (curnode == null) {
        ninfo.innerHTML = 'Must select a node below for this function.';
        ninfo.style.display = 'block';
        document.NodePost.nodeops.selectedIndex = 0;
        return false;
    }
    var node = curnode.substr(4);
    var ncntl = document.getElementById('nodecntl');
    if (document.NodePost.nodeops.value == 'nam') {
        ninfo.innerHTML = 'Naming functions.';
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
        ncntl.innerHTML = nodegrp[node] + nodegrpgrp[node][1];
        ncntl.style.display = 'block';
    } else if (document.NodePost.nodeops.value == 'pol') {
        ninfo.innerHTML = 'Polling settings';
        ninfo.style.display = 'block';
        ncntl.innerHTML = nodepoll[node];
        ncntl.style.display = 'block';
        DoPoll();
    } else {
        ninfo.style.display = 'none';
        ncntl.style.display = 'none';
    }
    return true;
}

function DoGroup() {
    var node = curnode.substr(4);
    var ngrp = document.getElementById('nodegrp');

    ngrp.innerHTML = nodegrpgrp[node][document.NodePost.group.value];
    return true;
}

function DoGrpPost() {
    var posthttp;
    var params = 'fun=group&node=' + curnode + '&num=' + document.NodePost.group.value + '&groups=';
    var opts = document.NodePost.groups.options;
    var i;

    // Start loop at 1 because index 0 contains the empty "remove" option, selecting this empty label
    // on its own creates an empty list and thus removes all associations from a group
    for (i = 1; i < opts.length; i++)
        if (opts[i].selected) {
            params += opts[i].text + ',';
        }

    if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
        posthttp = new XMLHttpRequest();
    } else {
        posthttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST', 'grouppost.html', true);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.send(params);

    return false;
}

function DoPoll() {
    var node = curnode.substr(4);
    var npoll = document.getElementById('nodepoll');
    var polled = document.getElementById('polled');

    if (polled != null)
        npoll.innerHTML = nodepollpoll[node][polled.value];
    return true;
}

function DoPollPost() {
    var posthttp;
    var params = 'fun=poll&node=' + curnode + '&ids=';
    var opts = document.NodePost.polls.options;
    var i;

    for (i = 0; i < opts.length; i++)
        params += opts[i].id + ',';
    params += '&poll=';
    for (i = 0; i < opts.length; i++)
        if (opts[i].selected)
            params += '1,';
        else
            params += '0,';


    if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
        posthttp = new XMLHttpRequest();
    } else {
        posthttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    posthttp.open('POST', 'pollpost.html', true);
    posthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    posthttp.send(params);

    return false;
}

function TopoLoad(fun) {
    var params = 'fun=' + fun;
    topohttp.open('POST', 'topopost.html', true);
    topohttp.onreadystatechange = TopoReply;
    topohttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    topohttp.send(params);

    return false;
}

function TopoReply() {
    var xml;
    if (topohttp.readyState == 4 && topohttp.status == 200) {
        xml = topohttp.responseXML;
        var elems = xml.getElementsByTagName('topo');
        if (elems.length > 0) {
            var i;
            var id;
            var list;
            var elem = elems[0];
            for (i = 0; i < elem.childNodes.length; i++) {
                var child = elem.childNodes[i];
                if (child.nodeType != 1)
                    continue;
                if (child.tagName == 'node') {
                    id = child.getAttribute('id');
                    list = child.firstChild.nodeValue;
                    routes[id] = list.split(',');
                }
            }
            var stuff = '<tr><th>Nodes</th>';
            var topohead = document.getElementById('topohead');
            var node = nodes[i];
            for (i = 1; i < nodes.length; i++) {
                if (node == null)
                    continue;
                stuff += '<th>' + i + '</th>';
            }
            stuff += '</tr>'
            topohead.innerHTML = stuff;
            stuff = '';
            for (i = 1; i < nodes.length; i++) {
                if (node == null)
                    continue;
                stuff += '<tr><td style="vertical-align: top; text-decoration: underline; background-color: #FFFFFF;">' + i + '</td>';
                var j, k = 0;
                for (j = 1; j < nodes.length; j++) {
                    if (nodes[j] == null)
                        continue;
                    var route = routes[i];
                    if (route != undefined && k < route.length && j == route[k]) {
                        stuff += '<td>*</td>';
                        k++;
                    } else {
                        stuff += '<td>&nbsp;</td>';
                    }
                }
                stuff += '</tr>';
            }
            var topobody = document.getElementById('topobody');
            topobody.innerHTML = stuff;
        }
    }
}

function DisplayStatClass(t, n) {
    var scb = document.getElementById('statclassbody');
    var sn = document.getElementById('statnode');
    if (curclassstat != null) {
        var lastn = curclassstat.id.substr(8);
        if (n != lastn) {
            curclassstat.className = 'normal';
        }
    }
    if (curclassstat == null || t != curclassstat) {
        curclassstat = t;
        t.className = 'highlight';
        sn.style.width = '72%';
        scb.innerHTML = classstats[n];
        document.getElementById('statclass').style.display = 'block';
    } else {
        curclassstat = null;
        t.className = 'normal';
        sn.style.width = '100%';
        scb.innerHTML = '';
        document.getElementById('statclass').style.display = 'none';
    }
    return true;
}

function StatLoad(fun) {
    var params = 'fun=' + fun;
    stathttp.open('POST', 'statpost.html', true);
    stathttp.onreadystatechange = StatReply;
    stathttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    stathttp.send(params);

    return false;
}

function StatReply() {
    var xml;
    var elem;

    if (stathttp.readyState == 4 && stathttp.status == 200) {
        xml = stathttp.responseXML;
        elem = xml.getElementsByTagName('stats');
        if (elem.length > 0) {
            var errors = xml.getElementsByTagName('errors');
            var counts = xml.getElementsByTagName('counts');
            var infos = xml.getElementsByTagName('info');
            var error = errors[0];
            var cnt = error.childNodes.length;
            var count = counts[0];
            if (count.childNodes.length > cnt)
                cnt = count.childNodes.length;
            var info = infos[0];
            if (info.childNodes.length > cnt)
                cnt = info.childNodes.length;
            var stuff = '';
            var i;
            for (i = 0; i < cnt; i++) {
                if (i < error.childNodes.length)
                    if (error.childNodes[i].nodeType != 1)
                        continue;
                if (i < count.childNodes.length)
                    if (count.childNodes[i].nodeType != 1)
                        continue;
                if (i < info.childNodes.length)
                    if (info.childNodes[i].nodeType != 1)
                        continue;
                stuff += '<tr>';
                if (i < error.childNodes.length)
                    stuff += '<td style="text-align: right;">' + error.childNodes[i].getAttribute('label') + ': </td><td style="text-align: left;">' + error.childNodes[i].firstChild.nodeValue + '</td>';
                else
                    stuff += '<td>&nbsp;</td><td>&nbsp;</td>';
                if (i < count.childNodes.length)
                    stuff += '<td style="text-align: right;">' + count.childNodes[i].getAttribute('label') + ': </td><td style="text-align: left;">' + count.childNodes[i].firstChild.nodeValue + '</td>';
                else
                    stuff += '<td>&nbsp;</td><td>&nbsp;</td>';
                if (i < info.childNodes.length)
                    stuff += '<td style="text-align: right;">' + info.childNodes[i].getAttribute('label') + ': </td><td style="text-align: left;">' + info.childNodes[i].firstChild.nodeValue + '</td>';
                else
                    stuff += '<td>&nbsp;</td><td>&nbsp;</td>';
                stuff += '</tr>';
            }
            var statnetbody = document.getElementById('statnetbody');
            statnetbody.innerHTML = stuff;
            var nodes = xml.getElementsByTagName('node');
            var stuff = '';
            var oldnode = null;
            if (curclassstat != null)
                oldnode = curclassstat.id;
            for (var i = 0; i < nodes.length; i++) {
                var node = nodes[i];
                stuff += '<tr id="statnode' + i + '" onclick="return DisplayStatClass(this,' + i + ');"><td>' + node.getAttribute('id') + '</td>';
                var nstat = node.getElementsByTagName('nstat');
                for (var j = 0; j < nstat.length; j++) {
                    stuff += '<td>' + nstat[j].firstChild.nodeValue + '</td>';
                }
                stuff += '</tr>';
                var cstuff = '';
                var cclass = node.getElementsByTagName('commandclass');
                for (var j = 0; j < cclass.length; j++) {
                    cstuff += '<tr><td>' + cclass[j].getAttribute('name') + '</td>';
                    var cstat = cclass[j].getElementsByTagName('cstat');
                    for (var k = 0; k < cstat.length; k++) {
                        cstuff += '<td>' + cstat[k].firstChild.nodeValue + '</td>';
                    }
                    cstuff += '</tr>';
                }
                classstats[i] = cstuff;
            }
            var statnodebody = document.getElementById('statnodebody');
            statnodebody.innerHTML = stuff;
        }
        if (oldnode != null) {
            var scb = document.getElementById('statclassbody');
            scb.innerHTML = classstats[oldnode.substr(8)];
            curclassstat = document.getElementById(oldnode);
            curclassstat.className = 'highlight';
        }
    }
}

function TestHealLoad(fun) {
    var params = 'fun=' + fun;
    if (fun == 'test') {
        var cnt = document.getElementById('testnode');
        if (cnt.value.length == 0) {
            params += '&num=0';
        } else {
            params += '&num=' + cnt.value;
        }
        var cnt = document.getElementById('testmcnt');
        if (cnt.value.length == 0) {
            alert('Missing count value');
            return false;
        }
        params += '&cnt=' + cnt.value;
    } else if (fun == 'heal') {
        var cnt = document.getElementById('healnode');
        if (cnt.value.length == 0) {
            params += '&num=0';
        } else {
            params += '&num=' + cnt.value;
        }
        var check = document.getElementById('healrrs');
        if (check.checked)
            params += '&healrrs=1';
    }
    atsthttp.open('POST', 'thpost.html', true);
    atsthttp.onreadystatechange = TestHealReply;
    atsthttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    atsthttp.send(params);

    return false;
}

function TestHealReply() {
    var xml;
    var elem;

    if (atsthttp.readyState == 4 && atsthttp.status == 200) {
        xml = atsthttp.responseXML;
        var threport = document.getElementById('testhealreport');
        elem = xml.getElementsByTagName('test');
        if (elem.length > 0) {
            threport.innerHTML = '';
        }
        elem = xml.getElementsByTagName('heal');
        if (elem.length > 0) {
            threport.innerHTML = '';
        }
    }
}

function RequestAllConfig(n) {
    var params = 'fun=racp&node=' + n;
    racphttp.open('POST', 'confparmpost.html', true);
    racphttp.onreadystatechange = PollReply;
    racphttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    racphttp.send(params);

    return false;
}

function RequestAll(n) {
    var params = 'fun=racp&node=' + n;
    racphttp.open('POST', 'refreshpost.html', true);
    racphttp.onreadystatechange = PollReply;
    racphttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    racphttp.send(params);

    return false;
}

function quotestring(s) {
    return s.replace(/\'/g, "");
}

function return2br(dataStr) {
    return dataStr.replace(/(\r\n|[\r\n])/g, "<br />");
}

function boxsize(field) {
    if (field.length < 8)
        return 8;
    return field.length + 2;
}

function CreateOnOff(i, j, vid) {
    var data = '<tr><td style="float: right;"';
    var node = nodes[i];
    var value = node.values[j];
    if (value.help.length > 0)
        data += ' onmouseover="ShowToolTip(\'' + quotestring(value.help) + '\',0);" onmouseout="HideToolTip();"';
    data += '><label><span class="legend">' + value.label + ':&nbsp;</span></label></td><td><select id="' + vid + '" onchange="return DoValue(\'' + vid + '\');"'
    if (value.readonly)
        data += ' disabled="true"';
    if (value.help.length > 0)
        data += ' onmouseover="ShowToolTip(\'' + quotestring(value.help) + '\',0);" onmouseout="HideToolTip();"';
    data += '>';
    if (value.value == 'True')
        data += '<option value="off">Off</option><option value="on" selected="true">On</option>';
    else if (value.value == 'False')
        data += '<option value="off" selected="true">Off</option><option value="on">On</option>';
    else
        data += '<option value="---" selected="true">---</option><option value="off">Off</option><option value="on">On</option>';
    data += '</select></td><td><span class="legend">' + value.units + '</span></td></tr>';
    return data;
}

function CreateTextBox(i, j, vid) {
    var data = '<tr><td style="float: right;"';
    var node = nodes[i];
    var value = node.values[j];
    if (value.help.length > 0)
        data += ' onmouseover="ShowToolTip(\'' + quotestring(value.help) + '\',0);" onmouseout="HideToolTip();"';
    var trimmed_value = value.value.replace(/(\n\s*$)/, "");
    data += '><label><span class="legend">' + value.label + ':&nbsp;</span></label></td><td><input type="text" class="legend form-control" size="' + boxsize(trimmed_value) + '" id="' + vid + '" value="' + trimmed_value + '"';
    if (value.help.length > 0)
        data += ' onmouseover="ShowToolTip(\'' + quotestring(value.help) + '\',0);" onmouseout="HideToolTip();"';
    if (value.readonly)
        data += ' disabled="true">';
    else
        data += '>';
    data += '<span class="legend">' + value.units + '</span>';
    if (!value.readonly)
        data += '<button class="btn btn-default" type="submit" onclick="return DoValue(\'' + vid + '\');">Submit</button>';
    data += '</td></tr>';
    return data;
}

function CreateList(i, j, vid) {
    var data = '<tr><td style="float: right;"';
    var node = nodes[i];
    var values = node.values[j];
    if (values.help.length > 0)
        data += ' onmouseover="ShowToolTip(\'' + quotestring(values.help) + '\',0);" onmouseout="HideToolTip();"';
    data += '><label><span class="legend">' + values.label + ':&nbsp;</span></label></td><td><select id="' + vid + '" onchange="return DoValue(\'' + vid + '\', false);"';
    if (values.help.length > 0)
        data += ' onmouseover="ShowToolTip(\'' + quotestring(values.help) + '\',0);" onmouseout="HideToolTip();"';
    if (values.readonly)
        data += ' disabled="true">';
    else
        data += '>';
    if (values.value != null)
        for (k = 0; k < values.value.length; k++) {
            data += '<option value="' + values.value[k].item + '"';
            if (values.value[k].selected)
                data += ' selected="true"';
            data += '>' + values.value[k].item + '</option>';
        }
    data += '</select><span class="legend">' + values.units + '</span></td></tr>';
    return data;
}

function CreateLabel(i, j, vid) {
    return '<tr><td style="float: right;"><label><span class="legend">' + nodes[i].values[j].label + ':&nbsp;</span></label></td><td><input type="text" class="legend form-control" disabled="true" size="' + boxsize(nodes[i].values[j].value) + '" id="' + vid + '" value="' + nodes[i].values[j].value + '"><span class="legend">' + nodes[i].values[j].units + '</span></td></tr>';
}

function CreateButton(i, j, vid) {
    var data = '<tr><td style="float: right;"';
    if (nodes[i].values[j].help.length > 0)
        data += ' onmouseover="ShowToolTip(\'' + quotestring(nodes[i].values[j].help) + '\',0);" onmouseout="HideToolTip();"';
    data += '><label><span class="legend">' + nodes[i].values[j].label + ':&nbsp;</span></label></td><td><button class="btn btn-default" type="submit" id="' + vid + '" onclick="return false;" onmousedown="return DoButton(\'' + vid + '\',true);" onmouseup="return DoButton(\'' + vid + '\',false);"'
    if (nodes[i].values[j].readonly)
        data += ' disabled="true"';
    data += '>Submit</button></td><td><span class="legend">' + nodes[i].values[j].units + '</span></td></tr>';
    return data;
}

function CreateDivs(genre, divtos, ind) {
    divto = '<table border="0" cellpadding="1" cellspacing="0"><tbody>';
    var node = nodes[ind];
    if (node.values != null) {
        var j = 0;
        for (var i = 0; i < node.values.length; i++) {
            var match;
            var value = node.values[i];
            if (genre == 'user')
                match = (value.genre == genre || value.genre == 'basic');
            else
                match = (value.genre == genre);
            if (!match)
                continue;
            var vid = node.id + '-' + value.cclass + '-' + value.genre + '-' + value.type + '-' + value.instance + '-' + value.index;
            j++;
            if (value.type == 'bool') {
                divto += CreateOnOff(ind, i, vid);
            } else if (value.type == 'byte') {
                divto += CreateTextBox(ind, i, vid);
            } else if (value.type == 'int') {
                divto += CreateTextBox(ind, i, vid);
            } else if (value.type == 'short') {
                divto += CreateTextBox(ind, i, vid);
            } else if (value.type == 'decimal') {
                divto += CreateTextBox(ind, i, vid);
            } else if (value.type == 'list') {
                divto += CreateList(ind, i, vid);
            } else if (value.type == 'string') {
                divto += CreateTextBox(ind, i, vid);
            } else if (value.type == 'button') {
                divto += CreateButton(ind, i, vid);
            } else if (value.type == 'raw') {
                divto += CreateTextBox(ind, i, vid);
            }
        }
        if (j != 0) {
            if (genre == 'config')
                divto += '<tr><td>&nbsp;</td><td><button class="btn btn-default" type="submit" id="requestallconfig" name="requestallconfig" onclick="return RequestAllConfig(' + ind + ');">Refresh</button></td><td>&nbsp;</td></tr>';
            else
                divto += '<tr><td>&nbsp;</td><td><button class="btn btn-default" type="submit" id="requestall" name="requestall" onclick="return RequestAll(' + ind + ');">Refresh</button></td><td>&nbsp;</td></tr>';
        }
    }
    divtos[ind] = divto + '</tbody></table>';
}

function CreateName(val, ind) {
    nodename[ind] = '<tr><td style="float: right;"><label><span class="legend">Name:&nbsp;</span></label></td><td><input type="text" class="legend form-control" size="' + boxsize(val) + '" id="name" value="' + val + '"><button class="btn btn-default" type="submit" style="margin-left: 5px;" onclick="return DoNodePost(document.NodePost.name.value);">Submit</button></td></tr>';
}

function CreateLocation(val, ind) {
    nodeloc[ind] = '<tr><td style="float: right;"><label><span class="legend">Location:&nbsp;</span></label></td><td><input type="text" class="legend form-control" size="' + boxsize(val) + '" id="location" value="' + val + '"><button class="btn btn-default" type="submit" style="margin-left: 5px;" onclick="return DoNodePost(document.NodePost.location.value);">Submit</button></td></tr>';
}

function CreateGroup(ind) {
    var grp;
    var i, j, k;

    if (nodes[ind].groups.length == 0) {
        nodegrp[ind] = '';
        nodegrpgrp[ind] = new Array();
        nodegrpgrp[ind][1] = '';
        return;
    }
    nodegrp[ind] = '<tr><td style="float: right;"><label><span class="legend">Groups:&nbsp;</span></label></td><td><select id="group" style="margin-left: 5px;" onchange="return DoGroup();">';
    nodegrpgrp[ind] = new Array(nodes[ind].groups.length);
    grp = 1;
    for (i = 0; i < nodes[ind].groups.length; i++) {
        nodegrp[ind] += '<option value="' + nodes[ind].groups[i].id + '">' + nodes[ind].groups[i].label + ' (' + nodes[ind].groups[i].id + ')</option>';
        // Add <option></option> at the start - this empty option allows to define/select an empty group
        nodegrpgrp[ind][grp] = '<td><div id="nodegrp" name="nodegrp" style="float: right;"><select id="groups" multiple size="8" style="vertical-align: top; margin-left: 5px;"><option></option>';
        k = 0;
        for (j = 1; j < nodes.length; j++) {
            var node = nodes[j];
            if (node == null)
                continue;

            // build a list of instances 
            var instances = [String(j)];
            for (var l = 0; l < node.values.length; l++) {
                instances.push(j + '.' + node.values[l].instance);
            }
            // On OpenZwave Version 1.6-845-gfe401290, july 2019  OZW adds node 1 endpoint 1 (assuming 1 is the controller)
            // That is actually instance "2"... So I add it (to enable its display)
			// See void MultiChannelAssociation::Set(uint8 _groupIdx, uint8 _targetNodeId, uint8 _instance)
			// Because ozwcp is in "low maintenance mode" and 99.9% of all controllers have ID 1... Hack it...
            if(j == 1)
                instances.push('1.2')

            // make unique
            instances = instances.filter(function (item, i, ar) {
                return ar.indexOf(item) === i;
            });

            // There used to be code to "only show when we have found multiple instances"
            // But I think it is clearer (and correct) to show all possible
            // single and multi instances

            if (nodes[ind].groups[i].nodes != null)
                while (k < nodes[ind].groups[i].nodes.length && nodes[ind].groups[i].nodes[k] < j)
                    k++;

            for (var l = 0; l < instances.length; l++) {
                if (nodes[ind].groups[i].nodes.indexOf(instances[l]) != -1)
                    nodegrpgrp[ind][grp] += '<option selected="true">' + instances[l] + '</option>';
                else
                    nodegrpgrp[ind][grp] += '<option>' + instances[l] + '</option>';
            }
        }
        nodegrpgrp[ind][grp] += '</select></td><td><button class="btn btn-default" type="submit" style="margin-left: 5px;" onclick="return DoGrpPost();">Submit</button></div></td></tr>';
        grp++;
    }
    nodegrp[ind] += '</select></td>';
}

function CreatePoll(ind) {
    var uc = 0;
    var sc = 0;
    var node = nodes[ind];
    if (node.values != null)
        for (var i = 0; i < node.values.length; i++) {
            if (node.values[i].genre == 'user')
                uc++;
            if (node.values[i].genre == 'system')
                sc++;
        }
    if (uc > 0 || sc > 0)
        nodepoll[ind] = '<tr><td style="float: right;"><label><span class="legend">Polling&nbsp;</span></label></td><td><select id="polled" style="margin-left: 5px;" onchange="return DoPoll();"><option value="0">User</option><option value="1">System</option></select></td><td><div id="nodepoll" name="nodepoll" style="float: left;"></div></td><td><button class="btn btn-default" type="submit" style="margin-left: 5px; vertical-align: top;" onclick="return DoPollPost();">Submit</button></td></tr>';
    else
        nodepoll[ind] = '';
    nodepollpoll[ind] = new Array(2);
    CreatePollPoll('user', ind, uc);
    CreatePollPoll('system', ind, sc);
}

function CreatePollPoll(genre, ind, cnt) {
    var ind1;
    if (genre == 'user')
        ind1 = 0;
    else
        ind1 = 1;
    var nodepoll = nodepollpoll[ind];
    nodepoll[ind1] = '<select id="polls" multiple size="4" style="vertical-align: top; margin-left: 5px;">';
    if (cnt > 0) {
        var node = nodes[ind];
        for (var i = 0; i < node.values.length; i++) {
            var value = node.values[i];
            if (value.genre != genre)
                continue;
            if (value.type == 'button')
                continue;
            var vid = node.id + '-' + value.cclass + '-' + genre + '-' + value.type + '-' + value.instance + '-' + value.index;
            nodepoll[ind1] += '<option id="' + vid + '"';
            if (value.polled)
                nodepoll[ind1] += ' selected="true"';
            nodepoll[ind1] += '>' + value.label + '</option>';
        }
        nodepoll[ind1] += '</select>';
    } else
        nodepoll[ind1] = '';
}
