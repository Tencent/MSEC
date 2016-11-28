
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


#ifndef __TCE_FUNCTION_H__
#define __TCE_FUNCTION_H__

namespace tce {

#define MADDR(x,y) &x::y

template <typename OWNER, typename FUNC>
struct BindMember0
{
	explicit BindMember0(OWNER& Owner, FUNC Func)
		: Owner_(&Owner), Func_(Func)
	{
	}

	void operator () () {
		(Owner_->*Func_)();
	}

	template <typename ARG>
	void operator () (ARG param) {
		(Owner_->*Func_)(param);
	}

	template <typename ARG1, typename ARG2>
	void operator () (ARG1 param1, ARG2 param2) {
		(Owner_->*Func_)(param1, param2);
	}

	template <typename ARG1, typename ARG2, typename ARG3>
	void operator () (ARG1 param1, ARG2 param2, ARG3 param3) {
		(Owner_->*Func_)(param1, param2, param3);
	}

	template <typename ARG1, typename ARG2, typename ARG3, typename ARG4>
	void operator () (ARG1 param1, ARG2 param2, ARG3 param3, ARG4 param4) {
		(Owner_->*Func_)(param1, param2, param3, param4);
	}		

private:
	OWNER * Owner_;
	FUNC Func_;
};

template <typename OWNER, typename FUNC, typename ARG>
struct BindMember1
{
	explicit BindMember1(OWNER& Owner, FUNC Func, ARG Param)
		: Owner_(&Owner), Func_(Func), Param_(Param)
	{
	}

	void operator () () {
		(Owner_->*Func_)(Param_);
	}

	template <typename ARG1>
	void operator () (ARG1 param) {
		(Owner_->*Func_)(Param_, param);
	}

	template <typename ARG1, typename ARG2>
	void operator () (ARG1 param1, ARG2 param2) {
		(Owner_->*Func_)(Param_, param1, param2);
	}

	template <typename ARG1, typename ARG2, typename ARG3>
	void operator () (ARG1 param1, ARG2 param2, ARG3 param3) {
		(Owner_->*Func_)(Param_, param1, param2, param3);
	}

	template <typename ARG1, typename ARG2, typename ARG3, typename ARG4>
	void operator () (ARG1 param1, ARG2 param2, ARG3 param3, ARG4 param4) {
		(Owner_->*Func_)(Param_, param1, param2, param3, param4);
	}		
	//	private:
	OWNER * Owner_;
	FUNC Func_;
	ARG Param_;
};

template <typename OWNER, typename FUNC, typename ARG1, typename ARG2>
struct BindMember2
{
	explicit BindMember2(OWNER& Owner, FUNC Func, ARG1 Param1, ARG2 Param2)
		: Owner_(&Owner), Func_(Func), Param1_(Param1), Param2_(Param2)
	{
	}

	void operator () () {
		(Owner_->*Func_)(Param1_, Param2_);
	}
private:
	OWNER * Owner_;
	FUNC Func_;
	ARG1 Param1_;
	ARG2 Param2_;
};	

};

#endif

