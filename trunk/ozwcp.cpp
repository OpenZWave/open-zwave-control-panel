//-----------------------------------------------------------------------------
//
//	ozwcp.cpp
//
//	OpenZWave Control Panel
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
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "Options.h"
#include "Manager.h"
#include "Node.h"
#include "Group.h"
#include "Notification.h"
#include "Log.h"

#include "microhttpd.h"
#include "ozwcp.h"
#include "webserver.h"

using namespace OpenZWave;

static Webserver *wserver;
pthread_mutex_t nlock = PTHREAD_MUTEX_INITIALIZER;
MyNode *nodes[MAX_NODES];
int32 MyNode::nodecount = 0;
pthread_mutex_t glock = PTHREAD_MUTEX_INITIALIZER;
bool done = false;
bool needsave = false;
uint32 homeId = 0;
uint8 nodeId = 0;
const char *cmode = "";
int32 debug = false;
bool MyNode::nodechanged = false;
list<uint8> MyNode::removed;

/*
 * MyNode::MyNode constructor
 * Just save the nodes into an array and other initialization.
 */
MyNode::MyNode (int32 const ind) : type(0)
{
  if (ind < 1 || ind > MAX_NODES) {
    Log::Write(LogLevel_Info, "new: bad node value %d, ignoring...", ind);
    delete this;
    return;
  }
  newGroup(ind);
  setTime(time(NULL));
  setChanged(true);
  nodes[ind] = this;
  nodecount++;
}

/*
 * MyNode::~MyNode destructor
 * Remove stored data.
 */
MyNode::~MyNode ()
{
  while (!values.empty()) {
    MyValue *v = values.back();
    values.pop_back();
    delete v;
  }
  while (!groups.empty()) {
    MyGroup *g = groups.back();
    groups.pop_back();
    delete g;
  }
}

/*
 * MyNode::remove
 * Remove node from array.
 */
void MyNode::remove (int32 const ind)
{
  if (ind < 1 || ind > MAX_NODES) {
    Log::Write(LogLevel_Info, "remove: bad node value %d, ignoring...", ind);
    return;
  }
  if (nodes[ind] != NULL) {
    addRemoved(ind);
    delete nodes[ind];
    nodes[ind] = NULL;
    nodecount--;
  }
}

/*
 * compareValue
 * Function to compare values in the vector for sorting.
 */
bool compareValue (MyValue *a, MyValue *b)
{
  return (a->getId() < b->getId());
}

/*
 * MyNode::sortValues
 * Sort the ValueIDs
 */
void MyNode::sortValues ()
{
  sort(values.begin(), values.end(), compareValue);
  setChanged(true);
}
/*
 * MyNode::addValue
 * Per notifications, add a value to a node.
 */
void MyNode::addValue (ValueID id)
{
  MyValue *v = new MyValue(id);
  values.push_back(v);
  setTime(time(NULL));
  setChanged(true);
}

/*
 * MyNode::removeValue
 * Per notification, remove value from node.
 */
void MyNode::removeValue (ValueID id)
{
  vector<MyValue*>::iterator it;
  bool found = false;
  for (it = values.begin(); it != values.end(); it++) {
    if ((*it)->id == id) {
      delete *it;
      values.erase(it);
      found = true;
      break;
    }
  }
  if (!found)
    fprintf(stderr, "removeValue not found Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s\n",
	    id.GetHomeId(), id.GetNodeId(), valueGenreStr(id.GetGenre()),
	    cclassStr(id.GetCommandClassId()), id.GetInstance(), id.GetIndex(),
	    valueTypeStr(id.GetType()));

  setChanged(true);
  setTime(time(NULL));
}

/*
 * MyNode::saveValue
 * Per notification, update value info. Nothing really but update
 * tracking state.
 */
void MyNode::saveValue (ValueID id)
{
  setTime(time(NULL));
  setChanged(true);
}

/*
 * MyNode::newGroup
 * Get initial group information about a node.
 */
void MyNode::newGroup (uint8 node)
{
  int n = Manager::Get()->GetNumGroups(homeId, node);
  for (int i = 1; i <= n; i++) {
    MyGroup *p = new MyGroup();
    p->groupid = i;
    p->max = Manager::Get()->GetMaxAssociations(homeId, node, i);
    p->label = Manager::Get()->GetGroupLabel(homeId, node, i);
    groups.push_back(p);
  }
}

/*
 * MyNode::addGroup
 * Add group membership based on notification updates.
 */
void MyNode::addGroup (uint8 node, uint8 g, uint8 n, uint8 *v)
{
  fprintf(stderr, "addGroup: node %d group %d n %d\n", node, g, n);
  if (groups.size() == 0)
    newGroup(node);
  for (vector<MyGroup*>::iterator it = groups.begin(); it != groups.end(); ++it)
    if ((*it)->groupid == g) {
      for (int i = 0; i < n; i++)
	(*it)->grouplist.push_back(v[i]);
      setTime(time(NULL));
      setChanged(true);
      return;
    }
  fprintf(stderr, "addgroup: node %d group %d not found in list\n", node, g);
}

/*
 * MyNode::getGroup
 * Return group ptr for XML output
 */
MyGroup *MyNode::getGroup (uint8 i)
{
  for (vector<MyGroup*>::iterator it = groups.begin(); it != groups.end(); ++it)
    if ((*it)->groupid == i)
      return *it;
  return NULL;
}

/*
 * MyNode::updateGroup
 * Synchronize changes from user and update to network
 */
void MyNode::updateGroup (uint8 node, uint8 grp, char *glist)
{
  char *p = glist;
  vector<MyGroup*>::iterator it;
  char *np;
  uint8 *v;
  uint8 n;
  uint8 j;

  fprintf(stderr, "updateGroup: node %d group %d\n", node, grp);
  for (it = groups.begin(); it != groups.end(); ++it)
    if ((*it)->groupid == grp)
      break;
  if (it == groups.end()) {
    fprintf(stderr, "updateGroup: node %d group %d not found\n", node, grp);
    return;
  }
  v = new uint8((*it)->max);
  n = 0;
  while (p != NULL && *p && n < (*it)->max) {
    np = strsep(&p, ",");
    v[n++] = strtol(np, NULL, 10);
  }
  /* Look for nodes in the passed-in argument list, if not present add them */
  vector<uint8>::iterator nit;
  for (j = 0; j < n; j++) {
    for (nit = (*it)->grouplist.begin(); nit != (*it)->grouplist.end(); ++nit)
      if (*nit == v[j])
	break;
    if (nit == (*it)->grouplist.end()) // not found
      Manager::Get()->AddAssociation(homeId, node, grp, v[j]);
  }
  /* Look for nodes in the vector (current list) and those not found in
     the passed-in list need to be removed */
  for (nit = (*it)->grouplist.begin(); nit != (*it)->grouplist.end(); ++nit) {
    for (j = 0; j < n; j++)
      if (*nit == v[j])
	break;
    if (j >= n)
      Manager::Get()->RemoveAssociation(homeId, node, grp, *nit);
  }
  delete [] v;
}

/*
 * Scan list of values to be added to/removed from poll list
 */
void MyNode::updatePoll(char *ilist, char *plist)
{
  vector<char*> ids;
  vector<bool> polls;
  MyValue *v;
  char *p;
  char *np;

  p = ilist;
  while (p != NULL && *p) {
    np = strsep(&p, ",");
    ids.push_back(np);
  }
  p = plist;
  while (p != NULL && *p) {
    np = strsep(&p, ",");
    polls.push_back(*np == '1' ? true : false);
  }
  if (ids.size() != polls.size()) {
    fprintf(stderr, "updatePoll: size of ids %d not same as size of polls %d\n",
	    ids.size(), polls.size());
    return;
  }
  vector<char*>::iterator it = ids.begin();
  vector<bool>::iterator pit = polls.begin();
  while (it != ids.end() && pit != polls.end()) {
    v = lookup(*it);
    if (v == NULL) {
      fprintf(stderr, "updatePoll: value %s not found\n", *it);
      continue;
    }
    /* if poll requested, see if not on list */
    if (*pit) {
      if (!Manager::Get()->isPolled(v->getId()))
	if (!Manager::Get()->EnablePoll(v->getId()))
	  fprintf(stderr, "updatePoll: enable polling for %s failed\n", *it);
    } else {			// polling not requested and it is on, turn it off
      if (Manager::Get()->isPolled(v->getId()))
	if (!Manager::Get()->DisablePoll(v->getId()))
	  fprintf(stderr, "updatePoll: disable polling for %s failed\n", *it);
    }
    ++it;
    ++pit;
  }
}

/*
 * Parse textualized value representation in the form of:
 * 2-SWITCH MULTILEVEL-user-byte-1-0
 * node-class-genre-type-instance-index
 */
MyValue *MyNode::lookup (string data)
{
  uint8 node = 0;
  uint8 cls;
  uint8 inst;
  uint8 ind;
  ValueID::ValueGenre vg;
  ValueID::ValueType typ;
  size_t pos1, pos2;
  string str;

  node = strtol(data.c_str(), NULL, 10);
  if (node == 0)
    return NULL;
  pos1 = data.find("-", 0);
  if (pos1 == string::npos)
    return NULL;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  cls = cclassNum(str.c_str());
  if (cls == 0xFF)
    return NULL;
  pos1 = pos2;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  vg = valueGenreNum(str.c_str());
  pos1 = pos2;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  typ = valueTypeNum(str.c_str());
  pos1 = pos2;
  pos2 = data.find("-", ++pos1);
  if (pos2 == string::npos)
    return NULL;
  str = data.substr(pos1, pos2 - pos1);
  inst = strtol(str.c_str(), NULL, 10);
  pos1 = pos2 + 1;
  str = data.substr(pos1);
  ind = strtol(str.c_str(), NULL, 10);
  ValueID id(homeId, node, vg, cls, inst, ind, typ);
  MyNode *n = nodes[node];
  if (n == NULL)
    return NULL;
  for (vector<MyValue*>::iterator it = n->values.begin(); it != n->values.end(); it++)
    if ((*it)->id == id)
      return *it;
  return NULL;
}

/*
 * Returns a count of values
 */
int32 MyNode::getValueCount ()
{
  return values.size();
}

/*
 * Returns an n'th value
 */
MyValue *MyNode::getValue (uint8 n)
{
  if (n < values.size())
    return values[n];
  return NULL;
}

/*
 * Returns next item on the removed list.
 */

uint8 MyNode::getRemoved()
{
  if (removed.size() > 0) {
    uint8 node = removed.front();
    removed.pop_front();
    return node;
  }
  return 0;
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void OnNotification (Notification const* _notification, void* _context)
{
  ValueID id = _notification->GetValueID();
  switch (_notification->GetType()) {
  case Notification::Type_ValueAdded:
    Log::Write(LogLevel_Info, "Notification: Value Added Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    nodes[_notification->GetNodeId()]->addValue(id);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_ValueRemoved:
    Log::Write(LogLevel_Info, "Notification: Value Removed Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    nodes[_notification->GetNodeId()]->removeValue(id);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_ValueChanged:
    Log::Write(LogLevel_Info, "Notification: Value Changed Home 0x%08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    nodes[_notification->GetNodeId()]->saveValue(id);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_Group:
    {
      Log::Write(LogLevel_Info, "Notification: Group Home 0x%08x Node %d Group %d",
		 _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetGroupIdx());
      uint8 *v;
      int8 n = Manager::Get()->GetAssociations(homeId, _notification->GetNodeId(), _notification->GetGroupIdx(), &v);
      pthread_mutex_lock(&nlock);
      nodes[_notification->GetNodeId()]->addGroup(_notification->GetNodeId(), _notification->GetGroupIdx(), n, v);
      pthread_mutex_unlock(&nlock);
      delete [] v;
    }
    break;
  case Notification::Type_NodeNew:
    Log::Write(LogLevel_Info, "Notification: Node New Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&glock);
    needsave = true;
    pthread_mutex_unlock(&glock);
    break;
  case Notification::Type_NodeAdded:
    Log::Write(LogLevel_Info, "Notification: Node Added Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    new MyNode(_notification->GetNodeId());
    pthread_mutex_unlock(&nlock);
    pthread_mutex_lock(&glock);
    needsave = true;
    pthread_mutex_unlock(&glock);
    break;
  case Notification::Type_NodeRemoved:
    Log::Write(LogLevel_Info, "Notification: Node Removed Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    MyNode::remove(_notification->GetNodeId());
    pthread_mutex_unlock(&nlock);
    pthread_mutex_lock(&glock);
    needsave = true;
    pthread_mutex_unlock(&glock);
    break;
  case Notification::Type_NodeProtocolInfo:
    Log::Write(LogLevel_Info, "Notification: Node Protocol Info Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    nodes[_notification->GetNodeId()]->saveValue(id);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_NodeNaming:
    Log::Write(LogLevel_Info, "Notification: Node Naming Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    nodes[_notification->GetNodeId()]->saveValue(id);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_NodeEvent:
    Log::Write(LogLevel_Info, "Notification: Node Event Home %08x Node %d Status %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetEvent(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    pthread_mutex_lock(&nlock);
    nodes[_notification->GetNodeId()]->saveValue(id);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_PollingDisabled:
    Log::Write(LogLevel_Info, "Notification: Polling Disabled Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    //pthread_mutex_lock(&nlock);
    //nodes[_notification->GetNodeId()]->setPolled(false);
    //pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_PollingEnabled:
    Log::Write(LogLevel_Info, "Notification: Polling Enabled Home %08x Node %d Genre %s Class %s Instance %d Index %d Type %s",
	       _notification->GetHomeId(), _notification->GetNodeId(),
	       valueGenreStr(id.GetGenre()), cclassStr(id.GetCommandClassId()), id.GetInstance(),
	       id.GetIndex(), valueTypeStr(id.GetType()));
    //pthread_mutex_lock(&nlock);
    //nodes[_notification->GetNodeId()]->setPolled(true);
    //pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_CreateButton:
    Log::Write(LogLevel_Info, "Notification: Create button Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
    break;
  case Notification::Type_DeleteButton:
    Log::Write(LogLevel_Info, "Notification: Delete button Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
    break;
  case Notification::Type_ButtonOn:
    Log::Write(LogLevel_Info, "Notification: Button On Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
    break;
  case Notification::Type_ButtonOff:
    Log::Write(LogLevel_Info, "Notification: Button Off Home %08x Node %d Button %d",
	       _notification->GetHomeId(), _notification->GetNodeId(), _notification->GetButtonId());
    break;
  case Notification::Type_DriverReady:
    Log::Write(LogLevel_Info, "Notification: Driver Ready, homeId %08x, nodeId %d", _notification->GetHomeId(),
	       _notification->GetNodeId());
    pthread_mutex_lock(&glock);
    homeId = _notification->GetHomeId();
    nodeId = _notification->GetNodeId();
    if (Manager::Get()->IsStaticUpdateController(homeId))
      cmode = "SUC";
    else if (Manager::Get()->IsPrimaryController(homeId))
      cmode = "Primary";
    else
      cmode = "Slave";
    pthread_mutex_unlock(&glock);
    break;
  case Notification::Type_DriverFailed:
    Log::Write(LogLevel_Info, "Notification: Driver Failed, homeId %08x", _notification->GetHomeId());
    pthread_mutex_lock(&glock);
    done = false;
    needsave = false;
    homeId = 0;
    cmode = "";
    pthread_mutex_unlock(&glock);
    pthread_mutex_lock(&nlock);
    for (int i = 1; i <= MAX_NODES; i++)
      MyNode::remove(i);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_DriverReset:
    Log::Write(LogLevel_Info, "Notification: Driver Reset, homeId %08x", _notification->GetHomeId());
    pthread_mutex_lock(&glock);
    done = false;
    needsave = false;
    homeId = _notification->GetHomeId();
    if (Manager::Get()->IsStaticUpdateController(homeId))
      cmode = "SUC";
    else if (Manager::Get()->IsPrimaryController(homeId))
      cmode = "Primary";
    else
      cmode = "Slave";
    pthread_mutex_unlock(&glock);
    pthread_mutex_lock(&nlock);
    for (int i = 1; i <= MAX_NODES; i++)
      MyNode::remove(i);
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_MsgComplete:
    Log::Write(LogLevel_Info, "Notification: Message Complete");
    break;
  case Notification::Type_EssentialNodeQueriesComplete:
    Log::Write(LogLevel_Info, "Notification: Essential Node %d Queries Complete", _notification->GetNodeId());
    break;
  case Notification::Type_NodeQueriesComplete:
    Log::Write(LogLevel_Info, "Notification: Node %d Queries Complete", _notification->GetNodeId());
    pthread_mutex_lock(&nlock);
    nodes[_notification->GetNodeId()]->sortValues();
    pthread_mutex_unlock(&nlock);
    break;
  case Notification::Type_AwakeNodesQueried:
    Log::Write(LogLevel_Info, "Notification: Awake Nodes Queried");
    break;
  case Notification::Type_AllNodesQueried:
    Log::Write(LogLevel_Info, "Notification: All Nodes Queried");
    break;
  default:
    Log::Write(LogLevel_Info, "Notification: type %d home %08x node %d genre %d class %d instance %d index %d type %d",
	       _notification->GetType(), _notification->GetHomeId(),
	       _notification->GetNodeId(), id.GetGenre(), id.GetCommandClassId(),
	       id.GetInstance(), id.GetIndex(), id.GetType());
    break;
  }
}

//-----------------------------------------------------------------------------
// <main>
// Create the driver and then wait
//-----------------------------------------------------------------------------
int32 main(int32 argc, char* argv[])
{
  int32 i;
  extern char *optarg;
  long webport;
  char *ptr;

  while ((i = getopt(argc, argv, "dp:")) != EOF)
    switch (i) {
    case 'd':
      debug = 1;
      break;
    case 'p':
      webport = strtol(optarg, &ptr, 10);
      if (ptr == optarg)
	goto bad;
      break;
    default:
    bad:
      fprintf(stderr, "usage: ozwcp [-d] -p <port>\n");
      exit(1);
    }

  for (i = 0; i < MAX_NODES; i++)
    nodes[i] = NULL;

  Options::Create("./config/", "", "--SaveConfiguration=true");
  Options::Get()->Lock();

  Manager::Create();
  Manager::Get()->AddWatcher(OnNotification, NULL);

  wserver = new Webserver(webport);
  while (!wserver->isReady()) {
    delete wserver;
    sleep(2);
    wserver = new Webserver(webport);
  }

  while (!done) {	// now wait until we are done
    sleep(1);
  }

  delete wserver;
  Manager::Get()->RemoveWatcher(OnNotification, NULL);
  Manager::Destroy();
  Options::Destroy();
  exit(0);
}
