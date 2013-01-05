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
#define EMPTY "<html></html>"

typedef struct _conninfo {
  conntype_t conn_type;
  char const *conn_url;
  void *conn_arg1;
  void *conn_arg2;
  void *conn_arg3;
  void *conn_arg4;
  void *conn_res;
  struct MHD_PostProcessor *conn_pp;
} conninfo_t;

bool Webserver::usb = false;
char *Webserver::devname = NULL;
unsigned short Webserver::port = 0;
bool Webserver::ready = false;

extern pthread_mutex_t nlock;
extern MyNode *nodes[];
extern pthread_mutex_t glock;
extern bool done;
extern bool needsave;
extern uint32 homeId;
extern uint8 nodeId;		// controller node id
extern uint8 SUCnodeId;
extern char *cmode;
extern bool noop;
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
int web_send_file (struct MHD_Connection *conn, const char *filename, const int code, const bool unl)
{
  struct stat buf;
  FILE *fp;
  struct MHD_Response *response;
  const char *p;
  const char *ct = NULL;
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
  if (unl)
    unlink(filename);
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
    for (uint j = 0; j < p->grouplist.size(); j++) {
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
 * web_get_values
 * Retreive class values based on genres
 */
void Webserver::web_get_values (int i, TiXmlElement *ep)
{
  int32 idcnt = nodes[i]->getValueCount();

  for (int j = 0; j < idcnt; j++) {
    TiXmlElement* valueElement = new TiXmlElement("value");
    MyValue *vals = nodes[i]->getValue(j);
    ValueID id = vals->getId();
    valueElement->SetAttribute("genre", valueGenreStr(id.GetGenre()));
    valueElement->SetAttribute("type", valueTypeStr(id.GetType()));
    valueElement->SetAttribute("class", cclassStr(id.GetCommandClassId()));
    valueElement->SetAttribute("instance", id.GetInstance());
    valueElement->SetAttribute("index", id.GetIndex());
    valueElement->SetAttribute("label", Manager::Get()->GetValueLabel(id).c_str());
    valueElement->SetAttribute("units", Manager::Get()->GetValueUnits(id).c_str());
    valueElement->SetAttribute("readonly", Manager::Get()->IsValueReadOnly(id) ? "true" : "false");
    if (id.GetGenre() != ValueID::ValueGenre_Config)
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
      if (id.GetType() == ValueID::ValueType_Decimal) {
	uint8 precision;
	if (Manager::Get()->GetValueFloatPrecision(id, &precision))
	  fprintf(stderr, "node = %d id = %d value = %s precision = %d\n", i, j, str.c_str(), precision);
      }
      valueElement->LinkEndChild(textElement);
    }

    string str = Manager::Get()->GetValueHelp(id);
    if (str.length() > 0) {
      TiXmlElement* helpElement = new TiXmlElement("help");
      TiXmlText *textElement = new TiXmlText(str.c_str());
      helpElement->LinkEndChild(textElement);
      valueElement->LinkEndChild(helpElement);
    }
    ep->LinkEndChild(valueElement);
  }
}

/*
 * SendTopoResponse
 * Process topology request and return appropiate data
 */

const char *Webserver::SendTopoResponse (struct MHD_Connection *conn, const char *fun,
					 const char *arg1, const char *arg2, const char *arg3)
{
  TiXmlDocument doc;
  char str[16];
  static char fntemp[32];
  char *fn;
  uint i, j, k;
  uint8 cnt;
  uint32 len;
  uint8 *neighbors;
  TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
  doc.LinkEndChild(decl);
  TiXmlElement* topoElement = new TiXmlElement("topo");
  doc.LinkEndChild(topoElement);

  if (strcmp(fun, "load") == 0) {
    cnt = MyNode::getNodeCount();
    i = 0;
    j = 1;
    while (j <= cnt && i < MAX_NODES) {
      if (nodes[i] != NULL) {
	len = Manager::Get()->GetNodeNeighbors(homeId, i, &neighbors);
	if (len > 0) {
	  TiXmlElement* nodeElement = new TiXmlElement("node");
	  snprintf(str, sizeof(str), "%d", i);
	  nodeElement->SetAttribute("id", str);
	  string list = "";
	  for (k = 0; k < len; k++) {
	    snprintf(str, sizeof(str), "%d", neighbors[k]);
	    list += str;
	    if (k < (len - 1))
	      list += ",";
	  }
	  fprintf(stderr, "topo: node=%d %s\n", i, list.c_str());
	  TiXmlText *textElement = new TiXmlText(list.c_str());
	  nodeElement->LinkEndChild(textElement);
	  topoElement->LinkEndChild(nodeElement);
	  delete [] neighbors;
	}
	j++;
      }
      i++;
    }
  }
  strncpy(fntemp, "/tmp/ozwcp.topo.XXXXXX", sizeof(fntemp));
  fn = mktemp(fntemp);
  if (fn == NULL)
    return EMPTY;
  strncat(fntemp, ".xml", sizeof(fntemp));
  if (debug)
    doc.Print(stdout, 0);
  doc.SaveFile(fn);
  return fn;
}

static TiXmlElement *newstat (char const *tag, char const *label, uint32 const value)
{
  char str[32];

  TiXmlElement* statElement = new TiXmlElement(tag);
  statElement->SetAttribute("label", label);
  snprintf(str, sizeof(str), "%d", value);
  TiXmlText *textElement = new TiXmlText(str);
  statElement->LinkEndChild(textElement);
  return statElement;
}

static TiXmlElement *newstat (char const *tag, char const *label, char const *value)
{
  TiXmlElement* statElement = new TiXmlElement(tag);
  statElement->SetAttribute("label", label);
  TiXmlText *textElement = new TiXmlText(value);
  statElement->LinkEndChild(textElement);
  return statElement;
}

/*
 * SendStatResponse
 * Process statistics request and return appropiate data
 */

const char *Webserver::SendStatResponse (struct MHD_Connection *conn, const char *fun,
					 const char *arg1, const char *arg2, const char *arg3)
{
  TiXmlDocument doc;
  static char fntemp[32];
  char *fn;

  TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
  doc.LinkEndChild(decl);
  TiXmlElement* statElement = new TiXmlElement("stats");
  doc.LinkEndChild(statElement);

  if (strcmp(fun, "load") == 0) {
    struct Driver::DriverData data;
    int i, j;
    int cnt;
    char str[16];

    Manager::Get()->GetDriverStatistics(homeId, &data);

    TiXmlElement* errorsElement = new TiXmlElement("errors");
    errorsElement->LinkEndChild(newstat("stat", "ACK Waiting", data.m_ACKWaiting));
    errorsElement->LinkEndChild(newstat("stat", "Read Aborts", data.m_readAborts));
    errorsElement->LinkEndChild(newstat("stat", "Bad Checksums", data.m_badChecksum));
    errorsElement->LinkEndChild(newstat("stat", "CANs", data.m_CANCnt));
    errorsElement->LinkEndChild(newstat("stat", "NAKs", data.m_NAKCnt));
    errorsElement->LinkEndChild(newstat("stat", "Out of Frame", data.m_OOFCnt));
    statElement->LinkEndChild(errorsElement);

    TiXmlElement* countsElement = new TiXmlElement("counts");
    countsElement->LinkEndChild(newstat("stat", "SOF", data.m_SOFCnt));
    countsElement->LinkEndChild(newstat("stat", "Total Reads", data.m_readCnt));
    countsElement->LinkEndChild(newstat("stat", "Total Writes", data.m_writeCnt));
    countsElement->LinkEndChild(newstat("stat", "ACKs", data.m_ACKCnt));
    countsElement->LinkEndChild(newstat("stat", "Total Broadcasts Received", data.m_broadcastReadCnt));
    countsElement->LinkEndChild(newstat("stat", "Total Broadcasts Transmitted", data.m_broadcastWriteCnt));
    statElement->LinkEndChild(countsElement);

    TiXmlElement* infoElement = new TiXmlElement("info");
    infoElement->LinkEndChild(newstat("stat", "Dropped", data.m_dropped));
    infoElement->LinkEndChild(newstat("stat", "Retries", data.m_retries));
    infoElement->LinkEndChild(newstat("stat", "Unexpected Callbacks", data.m_callbacks));
    infoElement->LinkEndChild(newstat("stat", "Bad Routes", data.m_badroutes));
    infoElement->LinkEndChild(newstat("stat", "No ACK", data.m_noack));
    infoElement->LinkEndChild(newstat("stat", "Network Busy", data.m_netbusy));
    infoElement->LinkEndChild(newstat("stat", "Not Idle", data.m_notidle));
    infoElement->LinkEndChild(newstat("stat", "Non Delivery", data.m_nondelivery));
    infoElement->LinkEndChild(newstat("stat", "Routes Busy", data.m_routedbusy));
    statElement->LinkEndChild(infoElement);

    cnt = MyNode::getNodeCount();
    i = 0;
    j = 1;
    while (j <= cnt && i < MAX_NODES) {
      struct Node::NodeData ndata;

      if (nodes[i] != NULL) {
	Manager::Get()->GetNodeStatistics(homeId, i, &ndata);
	TiXmlElement* nodeElement = new TiXmlElement("node");
	snprintf(str, sizeof(str), "%d", i);
	nodeElement->SetAttribute("id", str);
	nodeElement->LinkEndChild(newstat("nstat", "Sent messages", ndata.m_sentCnt));
	nodeElement->LinkEndChild(newstat("nstat", "Failed sent messages", ndata.m_sentFailed));
	nodeElement->LinkEndChild(newstat("nstat", "Retried sent messages", ndata.m_retries));
	nodeElement->LinkEndChild(newstat("nstat", "Received messages", ndata.m_receivedCnt));
	nodeElement->LinkEndChild(newstat("nstat", "Received duplicates", ndata.m_receivedDups));
	nodeElement->LinkEndChild(newstat("nstat", "Received unsolicited", ndata.m_receivedUnsolicited));
	nodeElement->LinkEndChild(newstat("nstat", "Last sent message", ndata.m_sentTS.substr(5).c_str()));
	nodeElement->LinkEndChild(newstat("nstat", "Last received message", ndata.m_receivedTS.substr(5).c_str()));
	nodeElement->LinkEndChild(newstat("nstat", "Last Request RTT", ndata.m_averageRequestRTT));
	nodeElement->LinkEndChild(newstat("nstat", "Average Request RTT", ndata.m_averageRequestRTT));
	nodeElement->LinkEndChild(newstat("nstat", "Last Response RTT", ndata.m_averageResponseRTT));
	nodeElement->LinkEndChild(newstat("nstat", "Average Response RTT", ndata.m_averageResponseRTT));
	nodeElement->LinkEndChild(newstat("nstat", "Quality", ndata.m_quality));
	while (!ndata.m_ccData.empty()) {
	  Node::CommandClassData ccd = ndata.m_ccData.front();
	  TiXmlElement* ccElement = new TiXmlElement("commandclass");
	  snprintf(str, sizeof(str), "%d", ccd.m_commandClassId);
	  ccElement->SetAttribute("id", str);
	  ccElement->SetAttribute("name", cclassStr(ccd.m_commandClassId));
	  ccElement->LinkEndChild(newstat("cstat", "Messages sent", ccd.m_sentCnt));
	  ccElement->LinkEndChild(newstat("cstat", "Messages received", ccd.m_receivedCnt));
	  nodeElement->LinkEndChild(ccElement);
	  ndata.m_ccData.pop_front();
	}
	statElement->LinkEndChild(nodeElement);
	j++;
      }
      i++;
    }
  }
  strncpy(fntemp, "/tmp/ozwcp.stat.XXXXXX", sizeof(fntemp));
  fn = mktemp(fntemp);
  if (fn == NULL)
    return EMPTY;
  strncat(fntemp, ".xml", sizeof(fntemp));
  if (debug)
    doc.Print(stdout, 0);
  doc.SaveFile(fn);
  return fn;
}

/*
 * SendTestHealResponse
 * Process network test and heal requests
 */

const char *Webserver::SendTestHealResponse (struct MHD_Connection *conn, const char *fun,
					     const char *arg1, const char *arg2, const char *arg3)
{
  TiXmlDocument doc;
  int node;
  int arg;
  bool healrrs = false;
  static char fntemp[32];
  char *fn;

  TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
  doc.LinkEndChild(decl);
  TiXmlElement* testElement = new TiXmlElement("testheal");
  doc.LinkEndChild(testElement);

  if (strcmp(fun, "test") == 0 && arg1 != NULL) {
    node = atoi((char *)arg1);
    arg = atoi((char *)arg2);
    if (node == 0)
      Manager::Get()->TestNetwork(homeId, arg);
    else
      Manager::Get()->TestNetworkNode(homeId, node, arg);
  } else if (strcmp(fun, "heal") == 0 && arg1 != NULL) {
    testElement = new TiXmlElement("heal");
    node = atoi((char *)arg1);
    if (arg2 != NULL) {
      arg = atoi((char *)arg2);
      if (arg != 0)
	healrrs = true;
    }
    if (node == 0)
      Manager::Get()->HealNetwork(homeId, healrrs);
    else
      Manager::Get()->HealNetworkNode(homeId, node, healrrs);
  }

  strncpy(fntemp, "/tmp/ozwcp.testheal.XXXXXX", sizeof(fntemp));
  fn = mktemp(fntemp);
  if (fn == NULL)
    return EMPTY;
  strncat(fntemp, ".xml", sizeof(fntemp));
  if (debug)
    doc.Print(stdout, 0);
  doc.SaveFile(fn);
  return fn;
}

/*
 * SendSceneResponse
 * Process scene request and return appropiate scene data
 */

const char *Webserver::SendSceneResponse (struct MHD_Connection *conn, const char *fun,
					  const char *arg1, const char *arg2, const char *arg3)
{
  TiXmlDocument doc;
  char str[16];
  string s;
  static char fntemp[32];
  char *fn;
  int cnt;
  int i;
  uint8 sid;
  TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
  doc.LinkEndChild(decl);
  TiXmlElement* scenesElement = new TiXmlElement("scenes");
  doc.LinkEndChild(scenesElement);

  if (strcmp(fun, "create") == 0) {
    sid = Manager::Get()->CreateScene();
    if (sid == 0) {
      fprintf(stderr, "sid = 0, out of scene ids\n");
      return EMPTY;
    }
  }
  if (strcmp(fun, "values") == 0 ||
      strcmp(fun, "label") == 0 ||
      strcmp(fun, "delete") == 0 ||
      strcmp(fun, "execute") == 0 ||
      strcmp(fun, "addvalue") == 0 ||
      strcmp(fun, "update") == 0 ||
      strcmp(fun, "remove") == 0) {
    sid = atoi((char *)arg1);
    if (strcmp(fun, "delete") == 0)
      Manager::Get()->RemoveScene(sid);
    if (strcmp(fun, "execute") == 0)
      Manager::Get()->ActivateScene(sid);
    if (strcmp(fun, "label") == 0)
      Manager::Get()->SetSceneLabel(sid, string(arg2));
    if (strcmp(fun, "addvalue") == 0 ||
	strcmp(fun, "update") == 0 ||
	strcmp(fun, "remove") == 0) {
      MyValue *val = MyNode::lookup(string(arg2));
      if (val != NULL) {
	if (strcmp(fun, "addvalue") == 0) {
	  if (!Manager::Get()->AddSceneValue(sid, val->getId(), string(arg3)))
	    fprintf(stderr, "AddSceneValue failure\n");
	} else if (strcmp(fun, "update") == 0) {
	  if (!Manager::Get()->SetSceneValue(sid, val->getId(), string(arg3)))
	    fprintf(stderr, "SetSceneValue failure\n");
	} else if (strcmp(fun, "remove") == 0) {
	  if (!Manager::Get()->RemoveSceneValue(sid, val->getId()))
	    fprintf(stderr, "RemoveSceneValue failure\n");
	}
      }
    }
  }
  if (strcmp(fun, "load") == 0 ||
      strcmp(fun, "create") == 0 ||
      strcmp(fun, "label") == 0 ||
      strcmp(fun, "delete") == 0) { // send all sceneids
    uint8 *sptr;
    cnt = Manager::Get()->GetAllScenes(&sptr);
    scenesElement->SetAttribute("sceneid", cnt);
    for (i = 0; i < cnt; i++) {
      TiXmlElement* sceneElement = new TiXmlElement("sceneid");
      snprintf(str, sizeof(str), "%d", sptr[i]);
      sceneElement->SetAttribute("id", str);
      s = Manager::Get()->GetSceneLabel(sptr[i]);
      sceneElement->SetAttribute("label", s.c_str());
      scenesElement->LinkEndChild(sceneElement);
    }
    delete [] sptr;
  }
  if (strcmp(fun, "values") == 0 ||
      strcmp(fun, "addvalue") == 0 ||
      strcmp(fun, "remove") == 0 ||
      strcmp(fun, "update") == 0) {
    vector<ValueID> vids;
    cnt = Manager::Get()->SceneGetValues(sid, &vids);
    scenesElement->SetAttribute("scenevalue", cnt);
    for (vector<ValueID>::iterator it = vids.begin(); it != vids.end(); it++) {
      TiXmlElement* valueElement = new TiXmlElement("scenevalue");
      valueElement->SetAttribute("id", sid);
      snprintf(str, sizeof(str), "0x%x", (*it).GetHomeId());
      valueElement->SetAttribute("home", str);
      valueElement->SetAttribute("node", (*it).GetNodeId());
      valueElement->SetAttribute("class", cclassStr((*it).GetCommandClassId()));
      valueElement->SetAttribute("instance", (*it).GetInstance());
      valueElement->SetAttribute("index", (*it).GetIndex());
      valueElement->SetAttribute("type", valueTypeStr((*it).GetType()));
      valueElement->SetAttribute("genre", valueGenreStr((*it).GetGenre()));
      valueElement->SetAttribute("label", Manager::Get()->GetValueLabel(*it).c_str());
      valueElement->SetAttribute("units", Manager::Get()->GetValueUnits(*it).c_str());
      Manager::Get()->SceneGetValueAsString(sid, *it, &s);
      TiXmlText *textElement = new TiXmlText(s.c_str());
      valueElement->LinkEndChild(textElement);
      scenesElement->LinkEndChild(valueElement);
    }
  }
  strncpy(fntemp, "/tmp/ozwcp.scenes.XXXXXX", sizeof(fntemp));
  fn = mktemp(fntemp);
  if (fn == NULL)
    return EMPTY;
  strncat(fntemp, ".xml", sizeof(fntemp));
  if (debug)
    doc.Print(stdout, 0);
  doc.SaveFile(fn);
  return fn;
}

/*
 * SendPollResponse
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
  if (nodeId != 0)
    snprintf(str, sizeof(str), "%d", nodeId);
  else
    str[0] = '\0';
  pollElement->SetAttribute("nodeid", str);
  snprintf(str, sizeof(str), "%d", SUCnodeId);
  pollElement->SetAttribute("sucnodeid", str);
  pollElement->SetAttribute("nodecount", MyNode::getNodeCount());
  pollElement->SetAttribute("cmode", cmode);
  pollElement->SetAttribute("save", needsave);
  pollElement->SetAttribute("noop", noop);
  if (noop)
    noop = false;
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

  TiXmlElement* updateElement = new TiXmlElement("update");
  pollElement->LinkEndChild(updateElement);
  i = MyNode::getRemovedCount();
  if (i > 0) {
    logbuffer[0] = '\0';
    while (i > 0) {
      uint8 node = MyNode::getRemoved();
      snprintf(str, sizeof(str), "%d", node);
      strcat(logbuffer, str);
      i = MyNode::getRemovedCount();
      if (i > 0)
	strcat(logbuffer, ",");
    }
    updateElement->SetAttribute("remove", logbuffer);
  }

  pthread_mutex_lock(&nlock);
  if (MyNode::getAnyChanged()) {
    i = 0;
    j = 1;
    while (j <= MyNode::getNodeCount() && i < MAX_NODES) {
      if (nodes[i] != NULL && nodes[i]->getChanged()) {
	bool listening;
	bool flirs;
	TiXmlElement* nodeElement = new TiXmlElement("node");
	pollElement->LinkEndChild(nodeElement);
	nodeElement->SetAttribute("id", i);
	nodeElement->SetAttribute("btype", nodeBasicStr(Manager::Get()->GetNodeBasic(homeId, i)));
	nodeElement->SetAttribute("gtype", Manager::Get()->GetNodeType(homeId, i).c_str());
	nodeElement->SetAttribute("name", Manager::Get()->GetNodeName(homeId, i).c_str());
	nodeElement->SetAttribute("location", Manager::Get()->GetNodeLocation(homeId, i).c_str());
	nodeElement->SetAttribute("manufacturer", Manager::Get()->GetNodeManufacturerName(homeId, i).c_str());
	nodeElement->SetAttribute("product", Manager::Get()->GetNodeProductName(homeId, i).c_str());
	listening = Manager::Get()->IsNodeListeningDevice(homeId, i);
	nodeElement->SetAttribute("listening", listening ? "true" : "false");
	flirs = Manager::Get()->IsNodeFrequentListeningDevice(homeId, i);
	nodeElement->SetAttribute("frequent", flirs ? "true" : "false");
	nodeElement->SetAttribute("beam", Manager::Get()->IsNodeBeamingDevice(homeId, i) ? "true" : "false");
	nodeElement->SetAttribute("routing", Manager::Get()->IsNodeRoutingDevice(homeId, i) ? "true" : "false");
	nodeElement->SetAttribute("security", Manager::Get()->IsNodeSecurityDevice(homeId, i) ? "true" : "false");
	nodeElement->SetAttribute("time", nodes[i]->getTime());
	fprintf(stderr, "i=%d failed=%d\n", i, Manager::Get()->IsNodeFailed(homeId, i));
	fprintf(stderr, "i=%d awake=%d\n", i, Manager::Get()->IsNodeAwake(homeId, i));
	fprintf(stderr, "i=%d state=%s\n", i, Manager::Get()->GetNodeQueryStage(homeId, i).c_str());
	fprintf(stderr, "i=%d listening=%d flirs=%d\n", i, listening, flirs);
	if (Manager::Get()->IsNodeFailed(homeId, i))
	  nodeElement->SetAttribute("status", "Dead");
	else {
	  string s = Manager::Get()->GetNodeQueryStage(homeId, i);
	  if (s == "Complete") {
	    if (i != nodeId && !listening && !flirs)
	      nodeElement->SetAttribute("status", Manager::Get()->IsNodeAwake(homeId, i) ? "Awake" : "Sleeping" );
	    else
	      nodeElement->SetAttribute("status", "Ready");
	  } else {
	    if (i != nodeId && !listening && !flirs)
	      s = s + (Manager::Get()->IsNodeAwake(homeId, i) ? " (awake)" : " (sleeping)");
	    nodeElement->SetAttribute("status", s.c_str());
	  }
	}
	web_get_groups(i, nodeElement);
	// Don't think the UI needs these
	//web_get_genre(ValueID::ValueGenre_Basic, i, nodeElement);
	web_get_values(i, nodeElement);
	nodes[i]->setChanged(false);
	j++;
      }
      i++;
    }
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
  ret = web_send_file(conn, fn, MHD_HTTP_OK, true);
  return ret;
}

/*
 * web_controller_update
 * Handle controller function feedback from library.
 */

void web_controller_update (Driver::ControllerState cs, Driver::ControllerError err, void *ct)
{
  Webserver *cp = (Webserver *)ct;
  string s;
  bool more = true;
 
  switch (cs) {
  case Driver::ControllerState_Normal:
    s = ": no command in progress.";
    break;
  case Driver::ControllerState_Starting:
    s = ": starting controller command.";
    break;
  case Driver::ControllerState_Cancel:
    s = ": command was cancelled.";
    more = false;
    break;
  case Driver::ControllerState_Error:
    s = ": command returned an error: ";
    more = false;
    break;
  case Driver::ControllerState_Sleeping:
    s = ": device went to sleep.";
    more = false;
    break;
  case Driver::ControllerState_Waiting:
    s = ": waiting for a user action.";
    break;
  case Driver::ControllerState_InProgress:
    s = ": communicating with the other device.";
    break;
  case Driver::ControllerState_Completed:
    s = ": command has completed successfully.";
    more = false;
    break;
  case Driver::ControllerState_Failed:
    s = ": command has failed.";
    more = false;
    break;
  case Driver::ControllerState_NodeOK:
    s = ": the node is OK.";
    more = false;
    break;
  case Driver::ControllerState_NodeFailed:
    s = ": the node has failed.";
    more = false;
    break;
  default:
    s = ": unknown respose.";
    break;
  }
  if (err != Driver::ControllerError_None)
    s  = s + controllerErrorStr(err);
  cp->setAdminMessage(s);
  cp->setAdminState(more);
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
  if (strcmp(cp->conn_url, "/devpost.html") == 0) {
    if (strcmp(key, "fn") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "dev") == 0)
      cp->conn_arg2 = (void *)strdup(data);
    else if (strcmp(key, "usb") == 0)
      cp->conn_arg3 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/valuepost.html") == 0) {
    cp->conn_arg1 = (void *)strdup(key);
    cp->conn_arg2 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/buttonpost.html") == 0) {
    cp->conn_arg1 = (void *)strdup(key);
    cp->conn_arg2 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/admpost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "node") == 0)
      cp->conn_arg2 = (void *)strdup(data);
    else if (strcmp(key, "button") == 0)
      cp->conn_arg3 = (void *)strdup(data);
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
  } else if (strcmp(cp->conn_url, "/scenepost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "id") == 0)
      cp->conn_arg2 = (void *)strdup(data);
    else if (strcmp(key, "vid") == 0)
      cp->conn_arg3 = (void *)strdup(data);
    else if (strcmp(key, "label") == 0)
      cp->conn_arg3 = (void *)strdup(data);
    else if (strcmp(key, "value") == 0)
      cp->conn_arg4 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/topopost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/statpost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/thpost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    if (strcmp(key, "num") == 0)
      cp->conn_arg2 = (void *)strdup(data);
    if (strcmp(key, "cnt") == 0)
      cp->conn_arg3 = (void *)strdup(data);
    if (strcmp(key, "healrrs") == 0)
      cp->conn_arg3 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/confparmpost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "node") == 0)
      cp->conn_arg2 = (void *)strdup(data);
  } else if (strcmp(cp->conn_url, "/refreshpost.html") == 0) {
    if (strcmp(key, "fun") == 0)
      cp->conn_arg1 = (void *)strdup(data);
    else if (strcmp(key, "node") == 0)
      cp->conn_arg2 = (void *)strdup(data);
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
    fprintf(stderr, "%x: %s: \"%s\" conn=%x size=%d *ptr=%x\n", pthread_self(), method, url, conn, *up_data_size, *ptr);
  if (*ptr == NULL) {	/* do never respond on first call */
    cp = (conninfo_t *)malloc(sizeof(conninfo_t));
    if (cp == NULL)
      return MHD_NO;
    cp->conn_url = url;
    cp->conn_arg1 = NULL;
    cp->conn_arg2 = NULL;
    cp->conn_arg3 = NULL;
    cp->conn_arg4 = NULL;
    cp->conn_res = NULL;
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {
      cp->conn_pp = MHD_create_post_processor(conn, 1024, web_config_post, (void *)cp);
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
      ret = web_send_file(conn, "cp.html", MHD_HTTP_OK, false);
    else if (strcmp(url, "/scenes.html") == 0)
      ret = web_send_file(conn, "scenes.html", MHD_HTTP_OK, false);
    else if (strcmp(url, "/cp.js") == 0)
      ret = web_send_file(conn, "cp.js", MHD_HTTP_OK, false);
    else if (strcmp(url, "/favicon.png") == 0)
      ret = web_send_file(conn, "openzwavetinyicon.png", MHD_HTTP_OK, false);
    else if (strcmp(url, "/poll.xml") == 0 && (devname != NULL || usb))
      ret = SendPollResponse(conn);
    else
      ret = web_send_data(conn, UNKNOWN, MHD_HTTP_NOT_FOUND, false, false, NULL); // no free, no copy
    return ret;
  } else if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {
    cp = (conninfo_t *)*ptr;
    if (strcmp(url, "/devpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (strcmp((char *)cp->conn_arg1, "open") == 0) { /* start connection */
	  if (devname != NULL || usb) {
	    MyNode::setAnyChanged(true);
	  } else {
	    if ((char *)cp->conn_arg3 != NULL && strcmp((char *)cp->conn_arg3, "true") == 0) {
	      Manager::Get()->AddDriver("HID Controller", Driver::ControllerInterface_Hid );
	      usb = true;
	    } else {
	      devname = (char *)malloc(strlen((char *)cp->conn_arg2) + 1);
	      if (devname == NULL) {
		fprintf(stderr, "Out of memory open devname\n");
		exit(1);
	      }
	      usb = false;
	      strcpy(devname, (char *)cp->conn_arg2);
	      Manager::Get()->AddDriver(devname);
	    }
	  }
	} else if (strcmp((char *)cp->conn_arg1, "close") == 0) { /* terminate */
	  if (devname != NULL || usb)
	    Manager::Get()->RemoveDriver(devname ? devname : "HID Controller");
	  if (devname != NULL) {
	    free(devname);
	    devname = NULL;
	  }
	  usb = false;
	  homeId = 0;
	} else if (strcmp((char *)cp->conn_arg1, "reset") == 0) { /* reset */
	  Manager::Get()->ResetController(homeId);
	} else if (strcmp((char *)cp->conn_arg1, "sreset") == 0) { /* soft reset */
	  Manager::Get()->SoftReset(homeId);
	} else if (strcmp((char *)cp->conn_arg1, "exit") == 0) { /* exit */
	  pthread_mutex_lock(&glock);
	  if (devname != NULL || usb) {
	    Manager::Get()->RemoveDriver(devname ? devname : "HID Controller");
	    free(devname);
	    devname = NULL;
	    homeId = 0;
	    usb = false;
	  }
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
	    fprintf(stderr, "SetValue string failed type=%s\n", valueTypeStr(val->getId().GetType()));
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/buttonpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;
	MyValue *val = MyNode::lookup(string((char *)cp->conn_arg1));
	if (val != NULL) {
	  string arg = (char *)cp->conn_arg2;
	  if ((char *)cp->conn_arg2 != NULL && strcmp((char *)cp->conn_arg2, "true") == 0) {
	    if (!Manager::Get()->PressButton(val->getId()))
	      fprintf(stderr, "PressButton failed");
	  } else {
	    if (!Manager::Get()->ReleaseButton(val->getId()))
	      fprintf(stderr, "ReleaseButton failed");
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/scenepost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	cp->conn_res = (void *)SendSceneResponse(conn, (char *)cp->conn_arg1, (char *)cp->conn_arg2, (char *)cp->conn_arg3, (char *)cp->conn_arg4);
	return MHD_YES;
      } else
	ret = web_send_file(conn, (char *)cp->conn_res, MHD_HTTP_OK, true);
    } else if (strcmp(url, "/topopost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	cp->conn_res = (void *)SendTopoResponse(conn, (char *)cp->conn_arg1, (char *)cp->conn_arg2, (char *)cp->conn_arg3, (char *)cp->conn_arg4);
	return MHD_YES;
      } else
	ret = web_send_file(conn, (char *)cp->conn_res, MHD_HTTP_OK, true);
    } else if (strcmp(url, "/statpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	cp->conn_res = (void *)SendStatResponse(conn, (char *)cp->conn_arg1, (char *)cp->conn_arg2, (char *)cp->conn_arg3, (char *)cp->conn_arg4);
      } else
	ret = web_send_file(conn, (char *)cp->conn_res, MHD_HTTP_OK, true);
	return MHD_YES;
    } else if (strcmp(url, "/thpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	cp->conn_res = (void *)SendTestHealResponse(conn, (char *)cp->conn_arg1, (char *)cp->conn_arg2, (char *)cp->conn_arg3, (char *)cp->conn_arg4);
	return MHD_YES;
      } else
	ret = web_send_file(conn, (char *)cp->conn_res, MHD_HTTP_OK, true);
    } else if (strcmp(url, "/confparmpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;
	if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 0) {
	  uint8 node = strtol((char *)cp->conn_arg2, NULL, 10);
	  Manager::Get()->RequestAllConfigParams(homeId, node);
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/refreshpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;
	if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 0) {
	  uint8 node = strtol((char *)cp->conn_arg2, NULL, 10);
	  Manager::Get()->RequestNodeDynamic(homeId, node);
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/admpost.html") == 0) {
      if (*up_data_size != 0) {
	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (strcmp((char *)cp->conn_arg1, "cancel") == 0) { /* cancel controller function */
	  Manager::Get()->CancelControllerCommand(homeId);
	  setAdminState(false);
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
	} else if (strcmp((char *)cp->conn_arg1, "remd") == 0) {
	  setAdminFunction("Remove Device");
	  setAdminState(
			Manager::Get()->BeginControllerCommand(homeId,
							       Driver::ControllerCommand_RemoveDevice,
							       web_controller_update, this, true));
	} else if (strcmp((char *)cp->conn_arg1, "hnf") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Has Node Failed");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_HasNodeFailed,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "remfn") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Remove Failed Node");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_RemoveFailedNode,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "repfn") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
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
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Request Network Update");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_RequestNetworkUpdate,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "reqnnu") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Request Node Neighbor Update");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_RequestNodeNeighborUpdate,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "assrr") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Assign Return Route");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_AssignReturnRoute,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "delarr") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Delete All Return Routes");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_DeleteAllReturnRoutes,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "snif") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Send Node Information");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_SendNodeInformation,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "reps") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    setAdminFunction("Replication Send");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_ReplicationSend,
								 web_controller_update, this, true, node));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "addbtn") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4 &&
	      cp->conn_arg3 != NULL) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    uint8 button = strtol(((char *)cp->conn_arg3), NULL, 10);
	    setAdminFunction("Add Button");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_CreateButton,
								 web_controller_update, this, true, node, button));
	  }
	} else if (strcmp((char *)cp->conn_arg1, "delbtn") == 0) {
	  if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4 &&
	      cp->conn_arg3 != NULL) {
	    uint8 node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	    uint8 button = strtol(((char *)cp->conn_arg3), NULL, 10);
	    setAdminFunction("Delete Button");
	    setAdminState(
			  Manager::Get()->BeginControllerCommand(homeId,
								 Driver::ControllerCommand_DeleteButton,
								 web_controller_update, this, true, node, button));
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/nodepost.html") == 0) {
      if (*up_data_size != 0) {
	uint8 node;

	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4 && cp->conn_arg3 != NULL) {
	  node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	  if (strcmp((char *)cp->conn_arg1, "nam") == 0) { /* Node naming */
	    Manager::Get()->SetNodeName(homeId, node, (char *)cp->conn_arg3);
	  } else if (strcmp((char *)cp->conn_arg1, "loc") == 0) { /* Node location */
	    Manager::Get()->SetNodeLocation(homeId, node, (char *)cp->conn_arg3);
	  } else if (strcmp((char *)cp->conn_arg1, "pol") == 0) { /* Node polling */
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
    } else if (strcmp(url, "/grouppost.html") == 0) {
      if (*up_data_size != 0) {
	uint8 node;
	uint8 grp;

	MHD_post_process(cp->conn_pp, up_data, *up_data_size);
	*up_data_size = 0;

	if (cp->conn_arg2 != NULL && strlen((char *)cp->conn_arg2) > 4 &&
	    cp->conn_arg3 != NULL && strlen((char *)cp->conn_arg3) > 0) {
	  node = strtol(((char *)cp->conn_arg2) + 4, NULL, 10);
	  grp = strtol((char *)cp->conn_arg3, NULL, 10);
	  if (strcmp((char *)cp->conn_arg1, "group") == 0) { /* Group update */
	    pthread_mutex_lock(&nlock);
	    nodes[node]->updateGroup(node, grp, (char *)cp->conn_arg4);
	    pthread_mutex_unlock(&nlock);
	  }
	}
	return MHD_YES;
      } else
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
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
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
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
	ret = web_send_data(conn, EMPTY, MHD_HTTP_OK, false, false, NULL); // no free, no copy
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

Webserver::Webserver (int const wport) : sortcol(COL_NODE), logbytes(0), adminstate(false)
{
  fprintf(stderr, "webserver starting port %d\n", wport);
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
  if (wdata != NULL) {
    MHD_stop_daemon((MHD_Daemon *)wdata);
    wdata = NULL;
    ready = false;
  }
}
