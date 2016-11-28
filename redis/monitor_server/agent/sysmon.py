
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



#!/usr/bin/env python
#-*-coding=utf-8-*-
#
# Filename   : sysmon.py
# Description: Report Server Basic Information to Monitor
# Version    : 0.6
# Last Update: 2016-09-23

import os
import re
from time import time, sleep
import datetime

################### Configuration ######################
version = "0.6"
hd_list = ['/']		#需要监控的硬盘位置，对应df里的位置
if_list_filter = ['lo','docker0']	#需要过滤流量监控的网卡名，如无需监控lo和docker0的流量
reportcmd = "/msec/agent/monitor/sysmon_set"
interval = 60
########################################################

class ServerMonitor:
	def __init__(self):
		self.report_dict = {}

	# report to monitor
	def report(self, id, value):
		self.report_dict['sys.'+id] = value;

	def reportall(self):
		for k,v in self.report_dict.iteritems():
			cmd_report_monitor = "%s '%s' %s" % (reportcmd, k, v)
			os.system(cmd_report_monitor)

	#get harddisk use information
	def get_hd_use(self):
		cmd_get_hd_use = 'df'
		try:
			fp = os.popen(cmd_get_hd_use)
		except:
			ErrorInfo = r'get_hd_use_error'
			print ErrorInfo
			return ErrorInfo
		re_obj = re.compile(r'^/dev/.+\s+(?P<used>\d+)%\s+(?P<mount>.+)')
		hd_use = {}
		for line in fp:
			match = re_obj.search(line)
			if match is not None:
				hd_use[match.groupdict()['mount']] = match.groupdict()['used']
		fp.close()
		return hd_use
	
	#get harddisk IO information
	def get_hd_io(self):
		try:
			fp = open('/proc/diskstats', 'r')
			io_str = fp.read()
			fp.close()
		except:
			ErrorInfo = r'get_hd_io_error'
			print ErrorInfo
			return ErrorInfo
		hd_io = {}
		re_obj = re.compile(r'(?P<dev>vda1)\s(?P<rio>\d+)\s\d+\s(?P<rsect>\d+)\s(?P<ruse>\d+)\s(?P<wio>\d+)\s\d+\s(?P<wsect>\d+)\s(?P<wuse>\d+)\s\d+\s(?P<use>\d+)')
		match = re_obj.search(io_str)
		if match is not None:
			hd_io = match.groupdict()
		else:
			hd_io = r'get_hd_io_error'
		return hd_io
	#get memory use information
	def get_mem_use(self):
		try:
			fp = open('/proc/meminfo', 'r')
		except:
			ErrorInfo = 'get_mem_use_error'
			print ErrorInfo
			return ErrorInfo
		mem_use = {}
		tmp_dic = {}
		re_obj = re.compile(r'^(MemTotal|MemFree|Cached|Dirty|Mapped):\s+([0-9]+)')
		for line in fp:
			m = re_obj.search(line)
			if m is not None:
				tmp_dic[m.group(1)] = m.group(2)
		fp.close()
		if tmp_dic.has_key('Mapped'):
			mem_use['free'] = (long(tmp_dic['MemFree']) + long(tmp_dic['Cached']) - long(tmp_dic['Dirty']) - long(tmp_dic['Mapped']))/1024 
		else:
			mem_use['free'] = long(tmp_dic['MemFree']) / 1024 
		mem_use['used'] = (long(tmp_dic['MemTotal']) / 1024) - mem_use['free']
		return mem_use       
		
	#get shared memory use
	def get_shm_use(self):
		cmd_get_shm_use = "ipcs -mu"
		try:
			fp = os.popen(cmd_get_shm_use)
			shm_str = fp.read()
			fp.close()
		except:
			ErrorInfo = r'get_shm_use_error'
			print ErrorInfo
			return ErrorInfo
		re_obj = re.compile(r'segments allocated\s+(?P<shm_num>\d+)\npages allocated\s+(?P<shm_size>\d+)', re.S)
		match = re_obj.search(shm_str)
		if match is not None:
			shm_dict = match.groupdict()
		else:
			shm_dict = r'get_shm_use_error'
			print shm_dict
		return shm_dict

	#get cpu average load information
	def get_cpu_load(self):
		cmd_get_cpu_load = "awk '{print $1,$2,$3}' /proc/loadavg"
		
		try:
			cpu_load = os.popen(cmd_get_cpu_load).read().split()
		except:
			ErrorInfo = 'get_cpu_load_error'
			print ErrorInfo
			return ErrorInfo
		
		return cpu_load
		
	#get each cpu use information
	def get_cpu_use(self):
		cmd_get_cpu_num = "grep 'cpu' /proc/stat | awk '{print $1}'"
		ErrorInfo = r'get_cpu_info_error'
		try:
			fp = os.popen(cmd_get_cpu_num)
			cpus = fp.readlines()
			fp.close()
		except:			
			print ErrorInfo
			return ErrorInfo
		CPUS = [ cpu.strip() for cpu in cpus ]
		cmd_get_cpu_use = "grep 'cpu' /proc/stat"
		tmp = os.popen(cmd_get_cpu_use).read()
		cpu_use = {}
		for cpu in CPUS:
			info={}
			pattern = str(cpu) + r".*\n"
			m = re.search(pattern,tmp)
			if m is not None:
				list = re.split('\s+',m.group())
				try:
					info["idle"] = float(list[4])
					info["total"] = float(list[1]) + float(list[2]) + float(list[3]) + float(list[4]) + float(list[5]) + float(list[6]) + float(list[7])
				except:
					print cpu+":" + ErrorInfo
					return ErrorInfo
			cpu_use[cpu] = info
		return cpu_use

	
	#get udp connection information
	def get_udp(self):
		cmd_get_udp_info = "grep 'Udp:' /proc/net/snmp|tail -n1|awk -F':' '{print $2}'"
		try:
			fp = os.popen(cmd_get_udp_info)
			udp_str = fp.read()
			fp.close()
		except:
			ErrorInfo = r'get_udp_info_error'
			print ErrorInfo
			return ErrorInfo
		udp_info = udp_str.strip().split()
		return udp_info

	#get tcp connection information 
	def get_tcp(self):
		cmd_get_tcp_info = "grep 'Tcp:' /proc/net/snmp|tail -n1|awk -F':' '{print $2}'"
		try:
			fp = os.popen(cmd_get_tcp_info)
			tcp_str = fp.read()
			fp.close()
		except:
			ErrorInfo = r'get_tcp_info_error'
			print ErrorInfo
			return ErrorInfo
		tcp_info = tcp_str.strip().split()
		return tcp_info
	
	#get interface name
	def get_ifname(self):
		cmd_get_ifname = "for i in `ls /sys/class/net/`; do echo $i; done"
		try:
			fp = os.popen(cmd_get_ifname)
			intfs = fp.readlines()
			fp.close()
		except:
			ErrorInfo = 'get_cpu_info_error'
			print ErrorInfo
			return ErrorInfo
		return [ intf.strip() for intf in intfs ]
		
	#get interface netflow
	def get_netflow(self, net_if_name):
		cmd_get_netflow = "awk -F'[: ]+' 'BEGIN{ORS=\" \"}/"+net_if_name+"/{print $3,$4,$11,$12}' /proc/net/dev"
		try:
			fp = os.popen(cmd_get_netflow)
			net_str = fp.read()
			fp.close()
		except:
			ErrorInfo = r'get_netflow_error'
			print ErrorInfo
			return ErrorInfo
		net_info = net_str.strip().split()
		return net_info

	#calculate netflow during interval secends
	def cal_netflow(self, fst, aft):
		in_trf = long(aft[0]) - long(fst[0])
		if in_trf < 0 : in_trf = in_trf + 4294967296
		in_trf = in_trf // 1024
		in_pkg = long(aft[1]) - long(fst[1])
		if in_pkg < 0 : in_pkg = in_pkg + 4294967296
		out_trf = long(aft[2]) - long(fst[2])
		if out_trf < 0 : out_trf = out_trf + 4294967296
		out_trf = out_trf // 1024
		out_pkg = long(aft[3]) - long(fst[3])
		if out_pkg < 0 : out_pkg = out_pkg + 4294967296
		## return result    
		tmp = [ in_trf,in_pkg,out_trf,out_pkg ]
		net_flow = [ str(item).strip() for item in tmp ]
		return net_flow

if __name__ == "__main__":
	# mark start time
	start_time = time()
	# print version
	print "Start: %s (v%s)" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), version)
	# initial object mon
	mon = ServerMonitor()

	if_1 = {}
	if_2 = {}
	
	if_list = mon.get_ifname()
	if_list = [i for i in if_list if i not in if_list_filter ] #omit filter
	
	for ifname in if_list:
		if_1[ifname] = mon.get_netflow(ifname)
	# get udp statistic number
	udp_1 = mon.get_udp()
	# get tcp statistic number
	tcp_1 = mon.get_tcp()
	# get harddisk io 
	hd_io_1 = mon.get_hd_io()

	# get cpu use
	cpu_use_1 = mon.get_cpu_use()
	
	#sleep interval
	sleep(interval)
	
	#get cpu use again
	cpu_use_2 = mon.get_cpu_use()

	# report cpu_use
	if cpu_use_1 != r'get_cpu_info_error' and cpu_use_2 != r'get_cpu_info_error':
		for key in cpu_use_1.keys():
			mon.report(key.upper() + "_Used(%)", int((1 - (cpu_use_2[key]["idle"] - cpu_use_1[key]["idle"])/(cpu_use_2[key]["total"] - cpu_use_1[key]["total"]))*100))
	
	# get mem use
	mem_use = mon.get_mem_use()
	# report mem use
	if mem_use != r'get_mem_use_error':
		mon.report('Memory_Used(MB)',mem_use['used'])
		mon.report('Memory_Free(MB)',mem_use['free'])
	
	# get hd use
	hd_use = mon.get_hd_use()
	# report harddisk use
	if hd_use != r'get_hd_use_error':
		for key in hd_use.keys():
			if key in hd_list:
				mon.report("DIR_"+key+"_Used(%)", hd_use[key])
	# get hd io after 60s and calculate
	hd_io_2 = mon.get_hd_io()
	if hd_io_1 !=  r'get_hd_io_error' and hd_io_2 != r'get_hd_io_error':
		io_read_req = long(hd_io_2['rio']) - long(hd_io_1['rio']) 
		if io_read_req < 0 : io_read_req = io_read_req + 4294967296
		hd_read_req = io_read_req / 60
		hd_io_read = long(hd_io_2['rsect']) - long(hd_io_1['rsect']) 
		if hd_io_read < 0 : hd_io_read = hd_io_read + 4294967296
		hd_io_read = hd_io_read / 120
		io_write_req = long(hd_io_2['wio']) - long(hd_io_1['wio']) 
		if io_write_req < 0 : io_write_req = io_write_req + 4294967296
		hd_write_req = io_write_req / 60
		hd_io_write = long(hd_io_2['wsect']) - long(hd_io_1['wsect'])
		if hd_io_write < 0 : hd_io_write = hd_io_write + 4294967296
		hd_io_write = hd_io_write / 120
		io_read_use = long(hd_io_2['ruse']) - long(hd_io_1['ruse'])
		if io_read_use < 0 : io_read_use = io_read_use + 4294967296
		io_write_use = long(hd_io_2['wuse']) - long(hd_io_1['wuse'])
		if io_write_use < 0 : io_write_use = io_write_use + 4294967296
		await = (io_read_use + io_write_use) * 1000 / (io_read_req + io_write_req)
		io_use = long(hd_io_2['use']) - long(hd_io_1['use'])
		if io_use < 0 : io_use = io_use + 4294967296
		svctm = (io_use * 1000)/ (io_read_req + io_write_req)
		# report to status
		mon.report('IO_Read(KB/s)', hd_io_read)
		mon.report('IO_Write(KB/s)', hd_io_write)
		mon.report('IO_Read(Req/s)', hd_read_req)
		mon.report('IO_Write(Req/s)', hd_write_req)
		mon.report('IO_Avg_Wait(us)', await)
		mon.report('IO_Avg_SvcTime(us)', svctm)
		
	# get udp statistic number after 60s
	udp_2 = mon.get_udp()
	# calculate udp statistic number and report
	if udp_1 != 'get_udp_info_error' and udp_2 != 'get_udp_info_error':
		udp_InDatagrams = long(udp_2[0]) - long(udp_1[0])
		if udp_InDatagrams < 0 : udp_InDatagrams += 4294967296
		udp_NoPorts = long(udp_2[1]) - long(udp_1[1])
		if udp_NoPorts < 0 : udp_NoPorts += 4294967296
		udp_InErrors = long(udp_2[2]) - long(udp_1[2])
		if udp_InErrors < 0 : udp_InErrors += 4294967296
		udp_OutDatagrams = long(udp_2[3]) - long(udp_1[3])
		if udp_OutDatagrams < 0 : udp_OutDatagrams += 4294967296
		# report udp 
		mon.report('UDP_Sent(Pkt/m)',udp_OutDatagrams)
		#mon.report('UDP_NoPorts(Pkt/m)',udp_NoPorts)
		mon.report('UDP_Recv(Pkt/m)',udp_InDatagrams)
		mon.report('UDP_Errors(Pkt/m)',udp_InErrors)
		if len(udp_1) == 6:
			udp_RcvbufErrors = long(udp_2[4]) - long(udp_1[4])
			if udp_RcvbufErrors < 0 : udp_RcvbufErrors += 4294967296
			udp_SndbufErrors = long(udp_2[5]) - long(udp_1[5])
			if udp_SndbufErrors < 0 : udp_SndbufErrors += 4294967296
			mon.report('UDP_RecvbufError(Pkt/m)',udp_RcvbufErrors)
			mon.report('UDP_SndbufError(Pkt/m)',udp_SndbufErrors)

	# get tcp statistic number after 60s
	tcp_2 = mon.get_tcp()
	# calculate tcp statistic number and report
	if tcp_1 != 'get_tcp_info_error' and tcp_2 != 'get_tcp_info_error':
		tcp_ActiveOpens = long(tcp_2[4]) - long(tcp_1[4])
		if tcp_ActiveOpens < 0 : tcp_ActiveOpens += 4294967296
		tcp_PassiveOpens = long(tcp_2[5]) - long(tcp_1[5])
		if tcp_PassiveOpens < 0 : tcp_PassiveOpens += 4294967296
		tcp_AttemptFails = long(tcp_2[6]) - long(tcp_1[6])
		if tcp_AttemptFails < 0 : tcp_AttemptFails += 4294967296
		tcp_InSegs = long(tcp_2[9]) - long(tcp_1[9])
		if tcp_InSegs < 0 : tcp_InSegs += 4294967296
		tcp_OutSegs = long(tcp_2[10]) - long(tcp_1[10])
		tcp_RetransSegs = long(tcp_2[11]) - long(tcp_1[11])
		if tcp_RetransSegs < 0 : tcp_RetransSegs += 4294967296
		if tcp_OutSegs < 0 : tcp_OutSegs += 4294967296
		tcp_InErrs = long(tcp_2[12]) - long(tcp_1[12])
		if tcp_InErrs < 0 : tcp_InErrs += 4294967296
		tcp_RetransRatio = tcp_RetransSegs * 100 / tcp_OutSegs 
		tcp_CurrEstab = long(tcp_2[8])
		# report udp 
		mon.report('TCP_Recv(Pkt/m)',tcp_InSegs)
		mon.report('TCP_Sent(Pkt/m)',tcp_OutSegs)
		mon.report('TCP_Errors(Pkt/m)',tcp_InErrs)

	# get netflow after 60s and calculate
	for ifname in if_list:
		if_2[ifname] = mon.get_netflow(ifname)

	for ifname in if_list:
		# calculate tel netflow
		if if_1[ifname] != 'get_netflow_error' and if_2[ifname] != 'get_netflow_error':
			if len(if_1[ifname]) == 4 and len(if_2[ifname]) == 4:
				cal_net = mon.cal_netflow(if_1[ifname], if_2[ifname])
				# report tel netflow
				mon.report(ifname+"_In(KB/m)", cal_net[0])
				mon.report(ifname+"_In(Pkt/m)", cal_net[1])			
				mon.report(ifname+"_Out(KB/m)", cal_net[2])
				mon.report(ifname+"_Out(Pkt/m)", cal_net[3])
			
	#Get Shared Memory Information
	#	shm_dict = mon.get_shm_use()
	#if shm_dict != r'get_shm_use_error':
	#	shm_num = shm_dict['shm_num']
	#	shm_size = long(shm_dict['shm_size']) * 4
	#	mon.report('Shared_Memory(MB)',shm_size)
	#	mon.report('Shared_Memory_Num',shm_num)

	#Get CPU Average Load Information
	cpu_load = mon.get_cpu_load()
	if cpu_load != r'get_cpu_load_error':
		load = [ int(float(x) * 100) for x in cpu_load ]
		mon.report('CPU_LoadAvg_1Min(*100)', load[0])
		mon.report('CPU_LoadAvg_5Mins(*100)', load[1])
		mon.report('CPU_LoadAvg_15Mins(*100)', load[2])

	#mark end time 
	end_time = time()
	#calculate run time
	run_time = (long(end_time * 10) - long(start_time * 10)) / 10
	print "Run Time: %ss" % run_time
	mon.reportall()
