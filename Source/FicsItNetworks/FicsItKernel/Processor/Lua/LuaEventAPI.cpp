#include "LuaEventAPI.h"

#include "Network/Signals/FINSignalSender.h"
#include "Network/FINNetworkComponent.h"
#include "FicsItKernel/FicsItKernel.h"

#include "FGPowerCircuit.h"

#include "LuaProcessor.h"
#include "LuaInstance.h"
#include "Network/FINHookSubsystem.h"
#include "Network/FINNetworkTrace.h"

namespace FicsItKernel {
	namespace Lua {
		void luaListen(lua_State* L, FFINNetworkTrace o) {
			auto net = LuaProcessor::luaGetProcessor(L)->getKernel()->getNetwork();
			UObject* obj = *o;
			if (!IsValid(obj)) luaL_error(L, "object is not valid");
			if (obj->Implements<UFINSignalSender>()) {
				IFINSignalSender::Execute_AddListener(obj, o.Reverse());
				UFINSignalUtility::SetupSender(obj->GetClass());
				AFINHookSubsystem::GetHookSubsystem(obj)->ClassesWithSignals.Add(obj->GetClass());
			}

			// Hooks
			AFINHookSubsystem::GetHookSubsystem(obj)->AddListener(obj, o.Reverse());

			net->signalSenders.Add(o);
		}

		int luaListen(lua_State* L) {
			FLuaSyncCall SyncCall(L);
			int args = lua_gettop(L);

			for (int i = 1; i <= args; ++i) {
				FFINNetworkTrace trace;
				auto o = (UObject*)getObjInstance<UObject>(L, i, &trace);
				luaListen(L, trace / o);
			}
			return LuaProcessor::luaAPIReturn(L, 0);
		}

		int luaPullContinue(lua_State* L, int status, lua_KContext ctx) {
			int args = lua_gettop(L);

			return args - ctx;
		}

		int luaPull(lua_State* L) {
			FLuaSyncCall SyncCall(L);
			int args = lua_gettop(L);
			double t = 0.0;
			if (args > 0) t = lua_tonumber(L, 1);

			auto luaProc = LuaProcessor::luaGetProcessor(L);
			check(luaProc);
			int a = luaProc->doSignal(L);
			if (!a && !(args > 0 && lua_isinteger(L, 1) && lua_tointeger(L, 1) == 0)) {
				luaProc->timeout = t;
				luaProc->pullStart = std::chrono::high_resolution_clock::now();
				luaProc->pullState = (args > 0) ? 1 : 2;
				
				lua_yieldk(L, 0, args, luaPullContinue);
				return LuaProcessor::luaAPIReturn(L, 0);
			}
			luaProc->pullState = 0;
			return LuaProcessor::luaAPIReturn(L, a);
		}

		void luaIgnore(lua_State* L, FFINNetworkTrace o) {
			auto net = LuaProcessor::luaGetProcessor(L)->getKernel()->getNetwork();
			UObject* obj = *o;
			if (!IsValid(obj)) luaL_error(L, "object is not valid");
			if (obj->Implements<UFINSignalSender>()) {
				IFINSignalSender::Execute_RemoveListener(obj, o.Reverse());
				net->signalSenders.Remove(o);
			}

			// Hooks
			AFINHookSubsystem::GetHookSubsystem(obj)->RemoveListener(obj, *o.Reverse());
		}

		int luaIgnore(lua_State* L) {
			FLuaSyncCall SyncCall(L);
			int args = lua_gettop(L);

			for (int i = 1; i <= args; ++i) {
				FFINNetworkTrace trace;
				auto o = getObjInstance<UObject>(L, i, &trace);
				luaIgnore(L, trace / o);
			}
			return LuaProcessor::luaAPIReturn(L, 0);
		}

		int luaIgnoreAll(lua_State* L) {
			FLuaSyncCall SyncCall(L);
			auto net = LuaProcessor::luaGetProcessor(L)->getKernel()->getNetwork();
			TSet<FFINNetworkTrace> senders = net->signalSenders;
			for (FFINNetworkTrace sender : senders) {
				UObject* s = *sender;
				if (s) {
					FFINNetworkTrace listener = sender.Reverse();
					IFINSignalSender::Execute_RemoveListener(s, listener);
					AFINHookSubsystem::GetHookSubsystem(net->component)->RemoveListener(s, net->component);
				}
			}
			net->signalSenders.Empty();
			return LuaProcessor::luaAPIReturn(L, 1);
		}

		int luaClear(lua_State* L) {
			LuaProcessor::luaGetProcessor(L)->getKernel()->getNetwork()->clearSignals();
			return 0;
		}

		static const luaL_Reg luaEventLib[] = {
			{"listen", luaListen},
			{"pull", luaPull},
			{"ignore", luaIgnore},
			{"ignoreAll", luaIgnoreAll},
			{"clear", luaClear},
			{NULL,NULL}
		};

		void setupEventAPI(lua_State* L) {
			PersistSetup("Event", -2);
			lua_newtable(L);
			luaL_setfuncs(L, luaEventLib, 0);
			PersistTable("Lib", -1);
			lua_setglobal(L, "event");
		}
	}
}
