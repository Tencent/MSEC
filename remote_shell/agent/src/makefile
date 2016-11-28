
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



INC	=  -I../inc
LIB	= -L/usr/lib64/ -lcrypto /usr/lib64/libdl.a

remote_shell_aent:agent.o frozen.o  SendFileToAgent.o log.o GetFileFromAgent.o SendCmdsToAgentAndRun.o rsa.o
	gcc -static -o remote_shell_agent agent.o frozen.o  SendFileToAgent.o log.o GetFileFromAgent.o SendCmdsToAgentAndRun.o  rsa.o $(LIB)  

agent.o:agent.c
	gcc -c agent.c $(INC)
frozen.o:frozen.c
	gcc -c frozen.c $(INC)

rsa.o:rsa.c
	gcc -c rsa.c $(INC)
SendFileToAgent.o:SendFileToAgent.c
	gcc -c SendFileToAgent.c $(INC)
SendCmdsToAgentAndRun.o:SendCmdsToAgentAndRun.c
	gcc -c SendCmdsToAgentAndRun.c $(INC)
GetFileFromAgent.o:GetFileFromAgent.c
	gcc -c GetFileFromAgent.c $(INC)
log.o:log.c
	gcc -c log.c $(INC)

clean:
	rm *.o remoate_shell_agent
