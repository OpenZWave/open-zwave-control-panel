//
//	webserver.cpp -- libmicrohttpd web interface for ozwcp
//
//	Copyright (c) 2010 Greg Satz <satz@iranger.com>
//	All rights reserved.
//
// SOFTWARE NOTICE AND LICENSE
// This work (including software, documents, or other related items) is being 
// provided by the copyright holders under the following license. By obtaining,
// using and/or copying this work, you (the licensee) agree that you have read,
// understood, and will comply with the following terms and conditions:
//
// Permission to use, copy, and distribute this software and its documentation,
// without modification, for any purpose and without fee or royalty is hereby 
// granted, provided that you include the full text of this NOTICE on ALL
// copies of the software and documentation or portions thereof.
//
// THIS SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS 
// MAKE NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR 
// PURPOSE OR THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE 
// ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
//
// COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR 
// CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR 
// DOCUMENTATION.
//
// The name and trademarks of copyright holders may NOT be used in advertising 
// or publicity pertaining to the software without specific, written prior 
// permission.  Title to copyright in this software and any associated 
// documentation will at all times remain with copyright holders.
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "ValueBool.h"
#include "ValueByte.h"
#include "ValueDecimal.h"
#include "ValueInt.h"
#include "ValueList.h"
#include "ValueShort.h"
#include "ValueString.h"
#include "tinyxml.h"

#include "microhttpd.h"
#include "ozwcp.h"
#include "webserver.h"

using namespace OpenZWave;

#define NBSP(str)	(str) == NULL || (*str) == '\0' ? "&nbsp;" : (str)
#define BLANK(str)	(str) == NULL || (*str) == '\0' ? "" : (str)
#define FNF "<html><head><title>File not found</title></head><body>File not found: %s</body></html>"
#define UNKNOWN "<html><head><title>Nothingness</title></head><body>There is nothing here. Sorry.</body></html>\n"
#define DEFAULT "<script type=\"text/javascript\"> document.location.href='/';</script>"
#define DEVJS "var dev = '%s'; var usb = %s;\n"

typedef struct _conninfo {
  conntype_t conn_type;
  char const *conn_url;
  void *conn_arg1;
  void *conn_arg2;
  void *conn_arg3;
  void *conn_arg4;
  struct MHD_PostProcessor *conn_pp;
} conninfo_t;

extern pthread_mutex_t nlock;
extern MyNode *nodes[];
extern pthread_mutex_t glock;
extern bool done;
extern bool needsave;
extern uint32 homeId;
extern uint8 nodeId;
extern char *cmode;
extern int debug;

/*
 * web_send_data
 * Send internal HTML string
 */
static int web_send_data (struct MHD_Connection *connection, const char *data,
			  const int code, bool free, bool copy, const char *ct)
{
  struct MHD_Response *response;
  int ret;

  response = MHD_create_response_from_data(strlen(data), (void *)data, free ? MHD_YES : MHD_NO, copy ? MHD_YES : MHD_NO);
  if (response == NULL)
    return MHD_NO;
  if (ct != NULL)
    MHD_add_response_header(response, "Content-type", ct);
  ret = MHD_queue_response(connection, code, response);
  MHD_destroy_response(response);
  return ret;
}

/*
 * web_read_file
 * Read files and send them out
 */
ssize_t web_read_file (void *cls, uint64_t pos, char *buf, size_t max)
{
  FILE *file = (FILE *)cls;

  fseek(file, pos, SEEK_SET);
  return fread(buf, 1, max, file);
}

/*
 * web_close_file
 */
void web_close_file (void *cls)
{
  FILE *fp = (FILE *)cls;

  fclose(fp);
}

/*
 * web_send_file
 * Read files and send them out
 */
int web_send_file (struct MHD_Connection *conn, const char *filename, const int code)
{
  struct stat buf;
  FILE *fp;
  struct MHD_Response *response;
  const char *p;
  char *ct = NULL;
  int ret;

  if ((p = strchr(filename, '.')) != NULL) {
    p++;
    if (strcmp(p, "xml") == 0)
      ct = "text/xml";
    else if (strcmp(p, "js") == 0)
      ct = "text/javascript";
  }
  if (stat(filename, &buf) == -1 ||
      ((fp = fopen(filename, "r")) == NULL)) {
    if (strcmp(p, "xml") == 0)
      response = MHD_create_response_from_data(0, (void *)"", MHD_NO, MHD_NO);
    else {
      int len = strlen(FNF) + strlen(filename) - 1; // len(%s) + 1 for \0
      char *s = (char *)malloc(len);
      if (s == NULL) {
	fprintf(stderr, "Out of memory FNF\n");
	exit(1);
      }
      snprintf(s, len, FNF, filename);
      response = MHD_create_response_from_data(len, (void *)s, MHD_YES, MHD_NO); // free
    }
  } else
    response = MHD_create_response_from_callback(buf.st_size, 32 * 1024, &web_read_file, fp,
						 &web_close_file);
  if (response == NULL)
    return MHD_YES;
  if (ct != NULL)
    MHD_add_response_header(response, "Content-type", ct);
  ret = MHD_queue_response(conn, code, response);
  MHD_destroy_response(response);
  return ret;
}

/*
 * web_get_groups
 * Return some XML to carry node group associations
 */

void Webserver::web_get_groups (int n, TiXmlElement *ep)
{
  int cnt = nodes[n]->numGroups();
  int i;

  TiXmlElement* groupsElement = new TiXmlElement("groups");
  ep->LinkEndChild(groupsElement);
  groupsElement->SetAttribute("cnt", cnt);
  for (i = 1; i <= cnt; i++) {
    TiXmlElement* groupElement = new TiXmlElement("group");
    MyGroup *p = nodes[n]->getGroup(i);
    groupElement->SetAttribute("ind", i);
    groupElement->SetAttribute("max", p->max);
    groupElement->SetAttribute("label", p->label.c_str());
    string str = "";
    for (int j = 0; j < p->grouplist.size(); j++) {
      char s[12];
      snprintf(s, sizeof(s), "%d", p->grouplist[j]);
      str += s;
      if (j + 1 < p->grouplist.size())
	str += ",";
    }
    TiXmlText *textElement = new TiXmlText(str.c_str());
    groupElement->LinkEndChild(textElement);
    groupsElement->LinkEndChild(groupElement);
  }
}

/*
 * web_get_genre
 * Retreive class values based on genres
 */
void Webserver::web_get_genre (ValueID::ValueGenre vg, int i, TiXmlElement *ep)
{
  int32 idcnt = nodes[i]->getValueCount(vg);

  if (idcnt > 0) {
    char *vgstr = valueGenreStr(vg);
    int32 j;

    TiXmlElement* genreElement = new TiXmlElement(vgstr);
    ep->LinkEndChild(genreElement);
    genreElement->SetAttribute("count", idcnt);
    for (j = 0; j < idcnt; j++) {
      MyValue *vals = nodes[i]->getValue(vg, j);
      ValueID id = vals->getId();
      TiXmlElement* valueElement = new TiXmlElement(valueTypeStr(id.GetType()));
      valueElement->SetAttribute("class", cclassStr(id.GetCommandClassId()));
      valueElement->SetAttribute("instance", id.GetInstance());
      valueElement->SetAttribute("index", id.GetIndex());
      valueElement->SetAttribute("label", Manager::Get()->GetValueLabel(id).c_str());
      valueElement->SetAttribute("units", Manager::Get()->GetValueUnits(id).c_str());
      valueElement->SetAttribute("readonly", Manager::Get()->IsValueReadOnly(id) ? "true" : "false");
      if (vg != ValueID::ValueGenre_Config)
	valueElement->SetAttribute("polled", Manager::Get()->isPolled(id) ? "true" : "false");
      if (id.GetType() == ValueID::ValueType_List) {
	vector<string> strs;
	Manager::Get()->GetValueListItems(id, &strs);
	valueElement->SetAttribute("count", strs.size());
	string str;
	Manager::Get()->GetValueListSelection(id, &str);
	valueElement->SetAttribute("current", str.c_str());
	for (vector<string>::iterator it = strs.begin(); it != strs.end(); it++) {
	  TiXmlElement* itemElement = new TiXmlElement("item");
	  valueElement->LinkEndChild(itemElement);
	  TiXmlText *textElement = new TiXmlText((*it).c_str());
	  itemElement->LinkEndChild(textElement);
	}
      } else {
	string str;
	TiXmlText *textElement;
	if (Manager::Get()->GetValueAsString(id, &str))
	  textElement = new TiXmlText(str.c_str());
	else
	  textElement = new TiXmlText("");
	valueElement->LinkEndChild(textElement);
      }
      genreElement->LinkEndChild(valueElement);
    }
  }
}

/*
 * web_send_poll
 * Process poll request from client and return
 * data as xml.
 */
int Webserver::SendPollResponse (struct MHD_Connection *conn)
{
  TiXmlDocument doc;
  struct stat buf;
  const int logbufsz = 1024;	// max amount to send of log per poll
  char logbuffer[logbufsz+1];
  off_t bcnt;
  char str[16];
  int32 i, j;
  int32 logread = 0;
  char fntemp[32];
  char *fn;
  FILE *fp;
  int32 ret;

  TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
  doc.LinkEndChild(decl);
  TiXmlElement* pollElement = new TiXmlElement("poll");
  doc.LinkEndChild(pollElement);
  if (homeId != 0L)
    snprintf(str, sizeof(str), "%08x", homeId);
  else
    str[0] = '\0';
  pollElement->SetAttribute("homeid", str);
  if (nodeId != 0L)
    snprintf(str, sizeof(str), "%d", nodeId);
  else
    str[0] = '\0';
  pollElement->SetAttribute("nodeid", str);
  pollElement->SetAttribute("nodecount", MyNode::getNodeCount());
  pollElement->SetAttribute("cmode", cmode);
  pollElement->SetAttribute("save", needsave);
  bcnt = logbytes;
  if (stat("./OZW_Log.txt", &buf) != -1 &&
      buf.st_size > bcnt &&
      (fp = fopen("./OZW_Log.txt", "r")) != NULL) {
    if (fseek(fp, bcnt, SEEK_SET) != -1) {
      logread = fread(logbuffer, 1, logbufsz, fp);
      while (logread > 0 && logbuffer[--logread] != '\n')
	;
      logbytes = bcnt + logread;
      fclose(fp);
    }
  }
  logbuffer[logread] = '\0';

  TiXmlElement* logElement = new TiXmlElement("log");
  pollElement->LinkEndChild(logElement);
  logElement->SetAttribute("size", logread);
  logElement->SetAttribute("offset", logbytes - logread);
  TiXmlText *textElement = new TiXmlText(logbuffer);
  logElement->LinkEndChild(textElement);

  TiXmlElement* adminElement = new TiXmlElement("admin");
  pollElement->LinkEndChild(adminElement);
  adminElement->SetAttribute("active", getAdminState() ? "true" : "false");
  if (adminmsg.length() > 0) {
    string msg = getAdminFunction() + getAdminMessage();
    TiXmlText *textElement = new TiXmlText(msg.c_str());
    adminElement->LinkEndChild(textElement);
    adminmsg.clear();
  }

  pthread_mutex_lock(&nlock);
  if (getNodesChanged()) {
    i = 0;
    j = 1;
    while (j <= MyNode::getNodeCount()) {
      if (nodes[i] != NULL) {
	TiXmlElement* nodeElement = new TiXmlElement("node");
	pollElement->LinkEndChild(nodeElement);
	nodeElement->SetAttribute("id", i);
	nodeElement->SetAttribute("btype", nodeBasicStr(Manager::Get()->GetNodeBasic(homeId, i)));
	nodeElement->SetAttribute("gtype", Manager::Get()->GetNodeType(homeId, i).c_str());
	nodeElement->SetAttribute("name", Manager::Get()->GetNodeName(homeId, i).c_str());
	nodeElement->SetAttribute("location", Manager::Get()->GetNodeLocation(homeId, i).c_str());
	nodeElement->SetAttribute("manufacturer", Manager::Get()->GetNodeManufacturerName(homeId, i).c_str());
	nodeElement->SetAttribute("product", Manager::Get()->GetNodeProductName(homeId, i).c_str());
	nodeElement->SetAttribute("time", nodes[i]->getTime());
	web_get_groups(i, nodeElement);
	// Don't think the UI needs these
	//web_get_genre(ValueID::ValueGenre_Basic, i, nodeElement);
	web_get_genre(ValueID::ValueGenre_User, i, nodeElement);
	web_get_genre(ValueID::ValueGenre_Config, i, nodeElement);
	web_get_genre(ValueID::ValueGenre_System, i, nodeElement);
	j++;
      }
      i++;
    }
    setNodesChanged(false);
  }
  pthread_mutex_unlock(&nlock);
  strncpy(fntemp, "/tmp/ozwcp.poll.XXXXXX", sizeof(fntemp));
  fn = mktemp(fntemp);
  if (fn == NULL)
    return MHD_YES;
  strncat(fntemp, ".xml", sizeof(fntemp));
  if (debug)
    doc.Print(stdout, 0);
  doc.SaveFile(fn);
  ret = web_send_file(conn, fn, MHD_HTTP_OK);
  unlink(fn);
  return ret;
}

/*
 * web_controller_update
 * Handle controller function feedback from library.
 */

void web_controller_update (Driver::ControllerState cs, void *ct)
{
  Webserver *cp = (Webserver *)ct;
 
  switch (cs) {
  case Driver::ControllerState_Normal:
    cp->setAdminMessage(": no command in progress.");
    break;
  case Driver::ControllerState_Waiting:
    cp->setAdminMessage(": waiting for a user action.");
    break;
  case Driver::ControllerState_InProgress:
    cp->setAdminMessage(": communicationg with the other device.");
    break;
  case Driver::ControllerState_Completed:
    cp->setAdminMessage(": command has completed successfully.");
    cp->setAdminState(false);
    break;
  case Driver::ControllerState_Failed:
    cp->setAdminMessage(": command has failed.");
    cp->setAdminState(false);
    break;
  case Driver::ControllerState_NodeOK:
    cp->setAdminMessage(": the node is OK.");
    cp->setAdminState(false);
    break;
  case Driver::ControllerState_NodeFailed:
    cp->setAdminMessage(": the node has failed.");
    cp->setAdminState(false);
    break;
  default:
    cp->setAdminMessage(": unknown respose.");
    break;
  }
}

/*
 * web_config_post
 * Handle the post of the updated data
 */

int web_config_post (void *cls, enum MHD_ValueKind kind, const char *key, const char *filename,
		     const char *content_type, const char *transfer_encoding,
		     const char *data, uint64_t off, size_t size)
{
  conninfo_t *cp = (conninfo_t *)cls;

  fprintf(stderr, "post: key=%s data=%s size=%d\n", key, data, size);
  if (strcmp(cp->conn_url, "/valuepost.html") == 0) {
    cp->conn_arg1 = (void *)strdup(key);
    cp->conn_arg2 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/admpost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "node") == 0)
      cp->conn_arg2 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/nodepost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "node") == 0)
      cp->conn_arg2 = (void *)strdup(data);
    else if (strcmp(key, "value") == 0)
      cp->conn_arg3 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/grouppost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "node") == 0)
      cp->conn_arg2 = (void *)strdup(data);
    else if (strcmp(key, "num") == 0)
      cp->conn_arg3 = (void *)strdup(data);
    else if (strcmp(key, "groups") == 0)
      cp->conn_arg4 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/pollpost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "node") == 0)
      cp->conn_arg2 = (void *)strdup(data);
    else if (strcmp(key, "ids") == 0)
      cp->conn_arg3 = (void *)strdup(data);
    else if (strcmp(key, "poll") == 0)
      cp->conn_arg4 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/savepost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
  }
  return MHD_YES;
}

/*
 * Process web requests
 */
int Webserver::HandlerEP (void *cls, struct MHD_Connection *conn, const char *url,
			const char *method, const char *version, const char *up_data,
			size_t *up_data_size, void **ptr)
{
  Webserver *ws = (Webserver *)cls;

  return ws->Handler(conn, url, method, version, up_data, up_data_size, ptr);
}

int Webserver::Handler (struct MHD_Connection *conn, const char *url,
			const char *method, const char *version, const char *up_data,
			size_t *up_data_size, void **ptr)
{
  int ret;
  conninfo_t *cp;

  if (debug)
    fprintf(stderr, "%s: \"%s\"\n", method, url);
  if (*ptr == NULL) {	/* do never respond on first call */
    cp = (conninfo_t *)malloc(sizeof(conninfo_t));
    if (cp == NULL)
      return MHD_NO;
    cp->conn_url = url;
    cp->conn_arg1 = NULL;
    cp->conn_arg2 = NULL;
    cp->conn_arg3 = NULL;
    cp->conn_arg4 = NULL;
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {
      cp->conn_pp = MHD_create_post_processor(conn, 512, web_config_post, (void *)cp);
      if (cp->conn_pp == NULL) {
	free(cp);
	return MHD_NO;
      }
      cp->conn_type = CON_POST;
    } else if (strcmp(method, MHD_HTTP_METHOD_GET) == 0) {
      cp->conn_type = CON_GET;
    } else {
      free(cp);
      return MHD_NO;
    }
    *ptr = (void *)cp;
    return MHD_YES;
  }
  if (strcmp(method, MHD_HTTP_METHOD_GET) == 0) {
    if (strcmp(url, "/") == 0 ||
	strcmp(url, "/index.html") == 0)
      ret = web_send_file(conn, "cp.html", MHD_HTTP_OK);
    else if (strcmp(url, "/cp.js") == 0)
      ret = web_send_file(conn, "cp.js", MHD_HTTP_OK);
    else if (strcmp(url, "/favicon.png") == 0)
      ret = web_send_file(conn, "openzwavetinyicon.png", MHD_HTTP_OK);
    else if (strcmp(url, "/dev.js") == 0) {
      int len = sizeof(DEVJS) - 1; // %s but one for \0
      char *s;
      if (devname != NULL) {
	len += strlen(devname);
	logbytes = 0;
	setNodesChanged(true);
      }
      s = (char *)malloc(len);
      if (s == NULL) {
	fprintf(stderr, "Out of memory dev.js\n");
	exit(1);
      }
      snprintf(s, len, DEVJS, devname ? devname : "", usb ? "true" : "false");
      ret = web_send_data(conn, s, MHD_HTTP_OK, true, false, "text/javascript"); // free but no copy
    } else if (strcmp(url, "/poll.xml") == 0 && (devname != NULL || usb)) {
      ret = SendPollResponse(conn);
    } else
      ret = web_send_data(conn, UNKNOWN, MHD_HTTP_NOT_FOUND, false, false, NULL); // no free, no copy
    return ret;
  } else if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {
    cp = (conninfo_t *)*ptr;
    if (strcmp(url, "/devpost.html") == 0) {
      const char *fun = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "fn");
      const char *dev = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "dev");
      const char *usbp = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "usb");
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (strcmp(fun, "open") == 0) { /* start connection */
	  if (devname != NULL) {
	    free(devname);
	    devname = NULL;
	  }
	  if (usbp != NULL && strcmp(usbp, "true") == 0) {
	    Manager::Get()->AddDriver("HID Controller", Driver::ControllerInterface_Hid );
	    usb = true;
	  } else {
	    devname = (char *)malloc(strlen(dev) + 1);
	    if (devname == NULL) {
	      fprintf(stderr, "Out of memory open devname\n");
	      exit(1);
	    }
	    usb = false;
	    strcpy(devname, dev);
	    Manager::Get()->AddDriver(devname);
	  }
	} else if (strcmp(fun, "close") == 0) { /* terminate */
	  if (devname != NULL || usb)
	    Manager::Get()->RemoveDriver(devname ? devname : "HID Controller");
	  if (devname != NULL) {
	    free(devname);
	    devname = NULL;
	  }
	  usb = false;
	  homeId = 0;
	} else if (strcmp(fun, "reset") == 0) { /* reset */
	  Manager::Get()->ResetController(homeId);
	  Manager::Get()->RemoveDriver(devname ? devname : "HID Controller");
	  sleep(5);
	  if (devname != NULL)
	    Manager::Get()->AddDriver(devname);
	  else
	    Manager::Get()->AddDriver("HID Controller", Driver::ControllerInterface_Hid );
	} else if (strcmp(fun, "sreset") == 0) { /* soft reset */
	  Manager::Get()->SoftReset(homeId);
	} else if (strcmp(fun, "exit") == 0) { /* exit */
	  pthread_mutex_lock(&glock);
	  Manager::Get()->RemoveDriver(devname ? devname : "HID Controller");
	  if (devname != NULL) {
	    free(devname);
	    devname = NULL;
	  }
	  homeId = 0;
	  usb = false;
	  done = true;						 // let main exit
	  pthread_mutex_unlock(&glock);
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, DEFAULT, MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/valuepost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;
	MyValue *val = MyNode::lookup(string((char *)cp->conn_arg1));
	if (val != NULL) {
	  string arg = (char *)cp->conn_arg2;
	  if (!Manager::Get()->SetValue(val->getId(), arg))
	    fprintf(stderr, "SetValue string failed");
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, "", MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/admpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (strcmp((char *)cp->conn_arg1, "cancel") == 0) { /* cancel controller function */
	  Manager::Get()->CancelControllerCommand(homeId);
	  setAdminState(false);
	} else if (strcmp((char *)cp->conn_arg1, "addc") == 0) {
	  setAdminFunction("Add Controller");
	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_AddController,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "addd") == 0) {
	  setAdminFunction("Add Device");
	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_AddDevice,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "cprim") == 0) {
	  setAdminFunction("Create Primary");







	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_CreateNewPrimary,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "rconf") == 0) {
	  setAdminFunction("Receive Configuration");
	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_ReceiveConfiguration,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "remc") == 0) {
	  setAdminFunction("Remove Controller");
	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_RemoveController,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "remd") == 0) {
	  setAdminFunction("Remove Device");
	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_RemoveDevice,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "hnf") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	    setAdminFunction("Has Node Failed");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_HasNodeFailed,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "remfn") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	    setAdminFunction("Remove Failed Node");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_RemoveFailedNode,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "repfn") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	    setAdminFunction("Replace Failed Node");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_ReplaceFailedNode,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "tranpr") == 0) {
	  setAdminFunction("Transfer Primary Role");
	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_TransferPrimaryRole,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "reqnu") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	    setAdminFunction("Request Network Update");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_RequestNetworkUpdate,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "reqnnu") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	    setAdminFunction("Request Node Neighbor Update");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_RequestNodeNeighborUpdate,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "assrr") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	    setAdminFunction("Assign Return Route");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_AssignReturnRoute,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "delarr") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	    setAdminFunction("Delete All Return Routes");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_DeleteAllReturnRoutes,
								 web_controller_update, this, true, node));
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, "", MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/nodepost.html") == 0) {
      if (*up_data_size != 0) {
	uint8 node;

	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4 && cp->conn_arg3 != NULL) {
	  node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	  if (strcmp((char *)cp->conn_arg1, "nam") == 0) { /* Node naming */
	    Manager::Get()->SetNodeName(homeId, node, (char *)cp->conn_arg3);
	  } else if (strcmp((char *)cp->conn_arg1, "loc") == 0) { /* Node location */
	    Manager::Get()->SetNodeLocation(homeId, node, (char *)cp->conn_arg3);
	  } else if (strcmp((char *)cp->conn_arg1, "pol") == 0) { /* Node polling */
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, "", MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/grouppost.html") == 0) {
      if (*up_data_size != 0) {
	uint8 node;
	uint8 grp;

	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4 &&
	    cp->conn_arg3 != NULL && strlen((char *)cp->conn_arg3) > 0 && cp->conn_arg4 != NULL) {
	  node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	  grp = strtol((char *)cp->conn_arg3, NULL, 10);
	  if (strcmp((char *)cp->conn_arg1, "group") == 0) { /* Group update */
	    pthread_mutex_lock(&nlock);
	    nodes[node]->updateGroup(node, grp, (char *)cp->conn_arg4);
	    pthread_mutex_unlock(&nlock);
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, "", MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/pollpost.html") == 0) {
      if (*up_data_size != 0) {
	uint8 node;

	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 0 &&
	    cp->conn_arg3 != NULL && strlen((char *)cp->conn_arg3) > 0 &&
	    cp->conn_arg4 != NULL && strlen((char *)cp->conn_arg4) > 0) {
	  node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10) + 1;
	  if (strcmp((char *)cp->conn_arg1, "poll") == 0) { /* Poll update */
	    pthread_mutex_lock(&nlock);
	    nodes[node]->updatePoll((char *)cp->conn_arg3, (char *)cp->conn_arg4);
	    pthread_mutex_unlock(&nlock);
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, "", MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/savepost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (strcmp((char *)cp->conn_arg1, "save") == 0) { /* Save config */
	  Manager::Get()->WriteConfig(homeId);
	  pthread_mutex_lock(&glock);
	  needsave = false;
	  pthread_mutex_unlock(&glock);
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, "", MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else
      ret = web_send_data(conn, UNKNOWN, MHD_HTTP_NOT_FOUND, false, false, NULL); // no free, no copy
    return ret;
  } else
    return MHD_NO;
}

/*
 * web_free
 * Free up any allocated data after connection closed
 */

void Webserver::FreeEP (void *cls, struct MHD_Connection *conn,
			void **ptr, enum MHD_RequestTerminationCode code)
{
  Webserver *ws = (Webserver *)cls;

  ws->Free(conn, ptr, code);
}

void Webserver::Free (struct MHD_Connection *conn, void **ptr, enum MHD_RequestTerminationCode code)
{
  conninfo_t *cp = (conninfo_t *)*ptr;

  if (cp != NULL) {
    if (cp->conn_arg1 != NULL)
      free(cp->conn_arg1);
    if (cp->conn_arg2 != NULL)
      free(cp->conn_arg2);
    if (cp->conn_arg3 != NULL)
      free(cp->conn_arg3);
    if (cp->conn_arg4 != NULL)
      free(cp->conn_arg4);
    if (cp->conn_type == CON_POST) {
      MHD_destroy_post_processor(cp->conn_pp);
    }
    free(cp);
    *ptr = NULL;
  }
}

/*
 * Constructor
 * Start up the web server
 */

Webserver::Webserver (int const wport) : ready(false), logbytes(0), nodeschanged(false), adminstate(false), sortcol(COL_NODE), devname(NULL), usb(false)
{
  port = wport;
  wdata = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG, port,
			   NULL, NULL, &Webserver::HandlerEP, this,
			   MHD_OPTION_NOTIFY_COMPLETED, Webserver::FreeEP, this, MHD_OPTION_END);

  if (wdata != NULL) {
    ready = true;
  }
}

/*
 * Destructor
 * Stop the web server
 */

Webserver::~Webserver ()
{
  if (wdata != NULL)
    MHD_stop_daemon((MHD_Daemon *)wdata);
}
