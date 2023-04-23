
//
//    Copyright 2023 Metehan Gezer
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//

#include "script/lua/w_scriptglue.hpp"
#include "util/w_logger.hpp"
#include "util/w_utils.hpp"
#include "rendering/w_mesh.hpp"
#include "scene/w_lights.hpp"
#include "input/w_input.hpp"

namespace Wiesel::ScriptGlue {

	void ReportErrors(lua_State *luaState, int status) {
		if (status == 0) {
			return;
		}
		LOG_ERROR("[SCRIPT ERROR] ", lua_tostring(luaState, -1));
		// remove error message from Lua state
		lua_pop(luaState, 1);
	}

	void ScriptVec3::Link(lua_State* L) {
		luabridge::getGlobalNamespace(L)
				.beginClass<glm::vec3>("RVec3") // read only vec3
					.addProperty("x", &glm::vec3::x, false)
					.addProperty("y", &glm::vec3::y, false)
					.addProperty("z", &glm::vec3::z, false)
				.endClass()
				.beginClass<ScriptVec3>("Vec3") // modifiable vec3
					.addProperty("x", &ScriptVec3::GetX, &ScriptVec3::SetX)
					.addProperty("y", &ScriptVec3::GetY, &ScriptVec3::SetY)
					.addProperty("z", &ScriptVec3::GetZ, &ScriptVec3::SetZ)
				.endClass();
	}

	void ScriptTransformComponent::Link(lua_State* L) {
		// todo scene class
		// todo entity class
		luabridge::getGlobalNamespace(L)
				.beginClass<ScriptTransformComponent>("TransformComponent")
					.addProperty("position", &ScriptTransformComponent::GetPosition)
					.addProperty("rotation", &ScriptTransformComponent::GetRotation)
					.addProperty("scale", &ScriptTransformComponent::GetScale)
					.addFunction("Move", &ScriptTransformComponent::Move, &ScriptTransformComponent::Move2)
					.addFunction("SetPosition", &ScriptTransformComponent::SetPosition, &ScriptTransformComponent::SetPosition2)
					.addFunction("Rotate", &ScriptTransformComponent::Rotate, &ScriptTransformComponent::Rotate2)
					.addFunction("SetRotation", &ScriptTransformComponent::SetRotation, &ScriptTransformComponent::SetRotation2)
					.addFunction("Resize", &ScriptTransformComponent::Resize, &ScriptTransformComponent::Resize2)
					.addFunction("SetScale", &ScriptTransformComponent::SetScale, &ScriptTransformComponent::SetScale2)
				.endClass();
	}

	std::map<std::string, LuaComponentGetFn> s_GetterFn = {};
	std::map<std::string, LuaModuleLoaderFn> s_ModuleLoader = {};

	template<class ScriptWrapper, class Component>
	void AddGetter(const std::string& name) {
		s_GetterFn[name] = [](Entity entity, lua_State* state) -> decltype(auto) {
			auto& transform = entity.GetComponent<Component>();
			luabridge::setGlobal(state, CreateReference<ScriptWrapper>(transform), "__currentcomponent");
			return luabridge::getGlobal(state, "__currentcomponent");
		};
	}

	LuaComponentGetFn GetComponentGetter(const char* name) {
		return s_GetterFn[std::string(name)];
	}

	void GenerateComponents() {
		AddGetter<ScriptTransformComponent, TransformComponent>("TransformComponent");
	}

	void RegisterModuleLoader(const std::string& name, LuaModuleLoaderFn fn) {
		s_ModuleLoader[name] = fn;
	}

	void GenerateModules() {
		RegisterModuleLoader("wiesel.def.lua", [](lua_State* state) {

		});
		RegisterModuleLoader("input.def.lua", [](lua_State* state) {
			luabridge::getGlobalNamespace(state)
					.beginNamespace("input")
						.addFunction("GetKey", &InputManager::GetKey)
						.addFunction("GetAxis", &InputManager::GetAxis)
						.addFunction("IsPressed", &InputManager::IsPressed)
					.endNamespace();
		});
	}

	luabridge::LuaRef StaticGetComponent(const std::string& str, lua_State* state, LuaBehavior* behavior) {
		return s_GetterFn[str](behavior->GetEntity(), state);
	}

	void RegisterModule(const char* name, lua_State* state) {
		if (!s_ModuleLoader.contains(name)) {
			throw std::runtime_error("Module with name do not exists " + std::string(name));
		}
		s_ModuleLoader[name](state);
	}

}