#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# Tencent is pleased to support the open source community by making MSEC available.
#
# Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
#
# Licensed under the GNU General Public License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. You may 
# obtain a copy of the License at
#
#     https://opensource.org/licenses/GPL-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the 
# License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific language governing permissions
# and limitations under the License.
#

import sys
import os
import time
import xml.dom.minidom
import yaml

def show_usage():
    print '''Usage: confsync.py [*.xml|*.yaml]
spp confsync version:1.0.0
A simple tool to sync spp xml and yaml config files'''

def getappname(ctrlxmlfile):
    fctrl = open(ctrlxmlfile, 'r')
    ctrldoc = xml.dom.minidom.parseString(fctrl.read())
    controllerNodes = ctrldoc.getElementsByTagName('controller')
    if len(controllerNodes) == 0:
        print 'Can not find <controller> node in spp_ctrl.xml'
        sys.exit()
    procmonNodes = controllerNodes[0].getElementsByTagName('procmon')
    if len(procmonNodes) == 0:
        print 'Can not find <procmon> node in spp_ctrl.xml'
        sys.exit()
    for groupNode in procmonNodes[0].getElementsByTagName('group'):
        if groupNode.getAttribute('id') == '0':
            proxybin = groupNode.getAttribute('exe')
            suffixindex = proxybin.find('_proxy')
            if suffixindex == -1:
                print 'ERROR:spp proxy bin filename[%s] is not ended with _proxy' % proxybin
                return None
            else:
                return proxybin[:suffixindex]

#get last modify time of spp xml files
def getsppxmlmtime(ctrlxmlfile):
    lastmtime = os.path.getmtime(ctrlxmlfile)

    fctrl = open(ctrlxmlfile, 'r')
    ctrldoc = xml.dom.minidom.parseString(fctrl.read())
    controllerNodes = ctrldoc.getElementsByTagName('controller')
    if len(controllerNodes) == 0:
        print 'Can not find <controller> node in spp_ctrl.xml'
        sys.exit()
    procmonNodes = controllerNodes[0].getElementsByTagName('procmon')
    if len(procmonNodes) == 0:
        print 'Can not find <procmon> node in spp_ctrl.xml'
        sys.exit()
    for groupNode in procmonNodes[0].getElementsByTagName('group'):
        filemtime = os.path.getmtime(groupNode.getAttribute('etc'))
        if(filemtime > lastmtime):
            lastmtime = filemtime
    return lastmtime

def write_proxy_xml_file(conf):
    proxyxmlfile = '../etc/spp_proxy.xml'
    dom = xml.dom.minidom.Document()
   
    root_element = dom.createElement('proxy')
    dom.appendChild(root_element)
    acceptor_element = dom.createElement('acceptor')
    root_element.appendChild(acceptor_element)
    connector_element = dom.createElement('connector')
    root_element.appendChild(connector_element)
    module_element = dom.createElement('module')
    root_element.appendChild(module_element)
    
    #parse <global> node
    if not conf.has_key('global'):
        print 'ERROR:node not found [global]'
        sys.exit()
    globallist = conf['global']
    if len(globallist) == 0:
        print 'ERROR:no config in [global]'
        sys.exit()

    has_listenconf = False
    for globalnode in globallist:
        if globalnode.has_key('listen'):
            listenconf = globalnode['listen']
            entry_element = dom.createElement('entry')
            index = listenconf.find(':')
            if index == -1:
                print 'ERROR:invalid listen config[%s]' % listenconf
                continue
            entry_element.setAttribute('if', listenconf[:index])
            slashindex = listenconf.find('/')
            if slashindex == -1:
                print 'ERROR:invalid listen config[%s]' % listenconf
                continue
            entry_element.setAttribute('port', listenconf[index + 1:slashindex])
            entry_element.setAttribute('type', listenconf[slashindex + 1:])
            acceptor_element.appendChild(entry_element)
            has_listenconf = True
    if not has_listenconf:
        print 'ERROR:node not found [listen]'
        sys.exit()
    
    if not conf.has_key('service'):
        print 'ERROR:node not found [service]'
        sys.exit()
    serviceconf = conf['service']
    for servicenode in serviceconf:
        entry_element = dom.createElement('entry')
        if not servicenode.has_key('id'):
            print 'ERROR:node not found [id]'
            sys.exit()
        entry_element.setAttribute('groupid', str(servicenode['id']))
        if servicenode.has_key('shmsize'):
            entry_element.setAttribute('send_size', str(servicenode['shmsize']))
            entry_element.setAttribute('recv_size', str(servicenode['shmsize']))
        connector_element.appendChild(entry_element)

        if(servicenode['id'] == 1):
            module_element.setAttribute('bin', servicenode['module'])
            if servicenode.has_key('conf'):
                module_element.setAttribute('etc', servicenode['conf'])
    
    fproxy = open(proxyxmlfile, 'w')
    fproxy.write(dom.toprettyxml(indent='\t', newl='\n', encoding='utf-8'))
    fproxy.close()
    
def write_worker_xml_file(groupconf):
    workerxmlfile = '../etc/spp_worker' + str(groupconf['id']) + '.xml'
    fworker = open(workerxmlfile, 'w')
    dom = xml.dom.minidom.Document()

    root_element = dom.createElement('worker')
    root_element.setAttribute('groupid', str(groupconf['id']))
    dom.appendChild(root_element)
    
    if groupconf.has_key('shmsize'):
        acceptor_element = dom.createElement('acceptor')
        entry_element = dom.createElement('entry')
        entry_element.setAttribute('send_size', str(groupconf['shmsize']))
        entry_element.setAttribute('recv_size', str(groupconf['shmsize']))
        acceptor_element.appendChild(entry_element)
        root_element.appendChild(acceptor_element)

    if groupconf.has_key('log'):
        logconf = groupconf['log']
        if logconf.has_key('level'):
            log_element = dom.createElement('log')
            log_element.setAttribute('level', str(logconf['level']))
            root_element.appendChild(log_element)

    module_element = dom.createElement('module')
    module_element.setAttribute('bin', groupconf['module'])
    if groupconf.has_key('conf'):
        module_element.setAttribute('etc', groupconf['conf'])
    root_element.appendChild(module_element)
    
    fworker.write(dom.toprettyxml(indent='\t', newl='\n', encoding='utf-8'))
    fworker.close()

def xml2yaml(ctrlxmlfile, yamlfile):
    print 'syncing xml to yaml...'
    #Init yaml
    yamlconf = dict()
    globalconf = list()
    serviceconf = list()

    #Parse ctrl xml file
    appname = getappname(ctrlxmlfile)
    if appname == None:
        print 'appname is None!'
        sys.exit()
    elif appname != 'spp':
        globalconf.append({'name': appname.encode('utf-8')})
    
    fctrl = open(ctrlxmlfile, 'r')
    ctrldoc = xml.dom.minidom.parseString(fctrl.read())
    controllerNodes = ctrldoc.getElementsByTagName('controller')
    if len(controllerNodes) == 0:
        print 'Can not find <controller> node in spp_ctrl.xml'
        sys.exit()
    procmonNodes = controllerNodes[0].getElementsByTagName('procmon')
    if len(procmonNodes) == 0:
        print 'Can not find <procmon> node in spp_ctrl.xml'
        sys.exit()
    fproxy = None
    for groupNode in procmonNodes[0].getElementsByTagName('group'):
        if groupNode.getAttribute('id') == '0':
            #Parse proxy xml file
            fproxy = open(groupNode.getAttribute('etc'), 'r')
            proxydoc = xml.dom.minidom.parseString(fproxy.read())
            proxyNodes = proxydoc.getElementsByTagName('proxy')
            if len(proxyNodes) == 0:
                print 'Can not find <proxy> node in spp_proxy.xml'
                sys.exit()
            acceptorNodes = proxyNodes[0].getElementsByTagName('acceptor')
            if len(acceptorNodes) == 0:
                print 'Can not find <acceptor> node in spp_proxy.xml'
                sys.exit()
            for entryNode in acceptorNodes[0].getElementsByTagName('entry'):
                entryValue = entryNode.getAttribute('if') + ':' + entryNode.getAttribute('port') + '/' + entryNode.getAttribute('type')
                globalconf.append({'listen': entryValue.encode('utf-8')})
        else: # Parse worker xml file
            fworker = open(groupNode.getAttribute('etc'), 'r')
            workerdoc = xml.dom.minidom.parseString(fworker.read())
            
            workerconf = dict()
            
            acceptorNodes = workerdoc.getElementsByTagName('acceptor')
            if len(acceptorNodes) > 0:
                entryNodes = acceptorNodes[0].getElementsByTagName('entry')
                if len(entryNodes) > 0:
                    workerconf['shmsize'] = int(entryNodes[0].getAttribute('recv_size'))

            logNodes = workerdoc.getElementsByTagName('log')
            if len(logNodes) > 0:
                log_level = int(logNodes[0].getAttribute('level'))
                workerconf['log'] = {'level': log_level}
    
            workerNodes = workerdoc.getElementsByTagName('worker')
            if len(workerNodes) == 0:
                print 'Can not find <worker> node in ' + groupNode.getAttribute('etc')
                sys.exit()
            moduleNodes = workerNodes[0].getElementsByTagName('module')
            if len(moduleNodes) == 0:
                print 'Can not find <module> node in' + groupNode.getAttribute('etc')
                sys.exit()
            workerconf['id'] = int(groupNode.getAttribute('id'))
            workerconf['module'] = moduleNodes[0].getAttribute('bin').encode('utf-8')
            if moduleNodes[0].hasAttribute('etc'):
                workerconf['conf'] = moduleNodes[0].getAttribute('etc').encode('utf-8')
            minprocnum = int(groupNode.getAttribute('minprocnum'))
            maxprocnum = int(groupNode.getAttribute('maxprocnum'))
            if minprocnum == maxprocnum:
                workerconf['procnum'] = minprocnum
            else:
                workerconf['minprocnum'] = minprocnum
                workerconf['maxprocnum'] = maxprocnum
            
            serviceconf.append(workerconf)
    yamlconf['global'] = globalconf
    yamlconf['service'] = serviceconf
    yamlstream = file(yamlfile, 'w')
    yaml.dump(yamlconf, yamlstream, encoding = 'utf-8', default_flow_style = False)
    
def yaml2xml(ctrlxmlfile, yamlfile):
    print 'syncing yaml to xml...'
    stream = file(yamlfile, 'r')
    confobj = yaml.load(stream)
  
    #parse <global> node
    appname = None
    if not confobj.has_key('global'):
        print 'ERROR:node not found [global]'
        sys.exit()
    globallist = confobj['global']
    for globalnode in globallist:
        if globalnode.has_key('name'):
            appname = globalnode['name']
    if appname == None:
        appname = 'spp' # default appname
    
    #init ctrl xml file
    dom = xml.dom.minidom.Document()
    
    root_element = dom.createElement('controller')
    procmon_element = dom.createElement('procmon')
    root_element.appendChild(procmon_element)
    dom.appendChild(root_element)
   
    proxy_group_element = dom.createElement('group')
    proxy_group_element.setAttribute('id', '0')
    proxy_group_element.setAttribute('exe', appname + '_proxy')
    proxy_group_element.setAttribute('etc', '../etc/spp_proxy.xml')
    proxy_group_element.setAttribute('maxprocnum', '1')
    proxy_group_element.setAttribute('minprocnum', '1')
    procmon_element.appendChild(proxy_group_element)
  
    #write proxy xml file
    write_proxy_xml_file(confobj)

    #parse <service> node
    if not confobj.has_key('service'):
        print 'ERROR:node not found [service]'
        sys.exit()
    serviceconf = confobj['service']
    for servicenode in serviceconf:
        worker_group_element = dom.createElement('group')
        if (not servicenode.has_key('procnum')) and (not servicenode.has_key('minprocnum') and not servicenode.has_key('maxprocnum')):
            print 'ERROR:node not found [procnum] or [minprocnum] or [maxprocnum]'
            sys.exit()
        if servicenode.has_key('procnum'):
            worker_group_element.setAttribute('minprocnum', str(servicenode['procnum']))
            worker_group_element.setAttribute('maxprocnum', str(servicenode['procnum']))
        for key in servicenode.keys():
            if key == 'id' or key == 'minprocnum' or key == 'maxprocnum':
                worker_group_element.setAttribute(key, str(servicenode[key]))
        worker_group_element.setAttribute('exe', appname + '_worker')
        workerxmlfile = '../etc/spp_worker' + str(servicenode['id']) + '.xml'
        worker_group_element.setAttribute('etc', workerxmlfile)
        procmon_element.appendChild(worker_group_element)
        write_worker_xml_file(servicenode) # write worker xml file
    
    #flush ctrl xml file
    fctrl = open(ctrlxmlfile, 'w')
    fctrl.write(dom.toprettyxml(indent='\t', newl='\n', encoding='utf-8'))
    fctrl.close()

if __name__=="__main__":    
    ctrlxmlfile = '../etc/spp_ctrl.xml'
    yamlfile = '../etc/service.yaml'

    if len(sys.argv) == 2:
        point = sys.argv[1].rfind('.')
        if point == -1:
            show_usage()
        else:
            suffix = sys.argv[1][point+1:] # get file suffix
            if suffix == 'xml':
                ctrlxmlfile = sys.argv[1]
            elif suffix == 'yaml':
                yamlfile = sys.argv[1]
            else:
                show_usage()
    elif len(sys.argv) > 2:
        show_usage()
        sys.exit()
    else:
        show_usage()

    # get modify time
    ctrlxmlmtime = None
    try:
        ctrlxmlmtime = os.path.getmtime(ctrlxmlfile)
    except Exception, data:
        print data
        print 'creating a new xml file...'
        ctrlxmlmtime = -1
    ymalmtime = None
    try:
        yamlmtime = os.path.getmtime(yamlfile)
    except Exception, data:
        if ctrlxmlmtime == -1:
            sys.exit()
        print data
        print 'creating a new yaml file...'
        yamlmtime = -1
    
    if ctrlxmlmtime == -1:
        yaml2xml(ctrlxmlfile, yamlfile)
    elif ctrlxmlmtime >= yamlmtime:
        xml2yaml(ctrlxmlfile, yamlfile)
    elif getsppxmlmtime(ctrlxmlfile) >= yamlmtime:
        xml2yaml(ctrlxmlfile, yamlfile)
    else:
        yaml2xml(ctrlxmlfile, yamlfile)

    print 'sync succeeded!'
