
/**
 * Tencent is pleased to support the open source community by making MSEC available.
 *
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the GNU General Public License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. You may 
 * obtain a copy of the License at
 *
 *     https://opensource.org/licenses/GPL-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the 
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */


<?php 

/**
 * @brief 框架自动生成的代码，请不要修改
 */

namespace service;

function loadclass()
{
	spl_autoload_register(function($className) {
			//运营机上固定目录
			$filename = "php/" . $className .".php";
			if (!file_exists($filename))
			{
				echo "Class file [$className.php] does not exist\n";
				return false;
			}
			else
			{
				require_once($filename);
			}

			return true;
		}
	);
}

function init($config)
{   
	loadclass();
	echo "service::init\n";
	return 0;
}   

function fini()
{   
	echo "service::fini\n";
	return 0;
}

/**
 * @brief 定时操作接口，使用者自己判断超时逻辑
 */
function loop()
{
    return 0;
}

function process($sMethod, $sReq, $isJson, $aExtInfo = array())
{
    $method_arr = explode(".", $sMethod);
    if (count($method_arr) < 3)
    {
        nglog_error("Invalid php method name: ". $sMethod);
        attr_report("Invalid method name");
        return -6; // -6 : invalid methodname
    }

    $class = $method_arr[count($method_arr)-2];
    $method = $method_arr[count($method_arr)-1];
    if (!class_exists($class))
    {
        nglog_error("Invalid php method name: ". $sMethod);
        attr_report("Invalid method name");
        return -6; // -6 : invalid methodname
    }

    $obj = new $class;
    if (!method_exists($obj, $method))
    {
        nglog_error("Invalid php method name: ". $sMethod);
        attr_report("Invalid method name");
        return -6; // -6 : invalid methodname
    }

    try {
        $sRsp = $obj->$method($sReq, $isJson);
    } catch (\Exception $e) {
        nglog_error("Php service handle process failed: [". $sMethod . "] :" . $e->getMessage());
        attr_report("Php handle exception: ". $sMethod);
        return -20;
    }

    return $sRsp;
}


//end of script

