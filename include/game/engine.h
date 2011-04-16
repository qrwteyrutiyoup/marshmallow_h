/*
 * Copyright 2011 Marshmallow Engine. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHMALLOW ENGINE ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARSHMALLOW ENGINE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Marshmallow Engine.
 */

#pragma once

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H 1

#include "core/shared.h"

MARSHMALLOW_NAMESPACE_BEGIN

namespace Event
{
	class EventManager;
	typedef Core::Shared<EventManager> SharedEventManager;
	
	class IEventListener;
	typedef Core::Shared<IEventListener> SharedEventListener;
}

namespace Game
{

	class SceneManager;
	typedef Core::Shared<SceneManager> SharedSceneManager;

	class EngineEventListener;

	/*! @brief Game Engine Class */
	class GAME_EXPORT Engine
	{
		static Engine *s_instance;
		Event::SharedEventManager  m_event_manager;
		Game::SharedSceneManager   m_scene_manager;
		Event::SharedEventListener m_event_listener;
		float  m_fps;
		float  m_ups;
		TIME   m_delta_time;
		int    m_exit_code;
		int    m_frame_rate;
		bool   m_running;

	public:

		/*!
		 * @param fps Desired frame rate
		 * @param ups Desired update rate (debug)
		 */
		Engine(float fps = 60.0, float ups = 60.0);
		virtual ~Engine(void);

		/*!
		 * @brief Start Engine
		 */
		int run(void);

		/*!
		 * @brief Stop Engine
		 * @param exit_code Exit code
		 */
		void stop(int exit_code = 0);

		/*!
		 * @brief Event Manager
		 */
		Event::SharedEventManager eventManager(void) const;

		/*!
		 * @brief Scene Manager
		 */
		SharedSceneManager sceneManager(void) const;

		/*!
		 * @brief Set Event Manager
		 */
		void setEventManager(Event::SharedEventManager &m);

		/*!
		 * @brief Set Scene Manager
		 */
		void setSceneManager(SharedSceneManager &m);

		/*!
		 * @brief Target frames per second
		 */
		float fps(void) const
		    { return(m_fps); }

		/*!
		 * @brief Target updates per second
		 */
		float ups(void) const
		    { return(m_ups); }

		/*!
		 * @brief Time that has elapsed since last tick
		 */
		TIME deltaTime(void) const
		    { return(m_delta_time); }

		/*!
		 * @brief Actual frame rate achieved
		 */
		int frameRate(void)
		    { return(m_frame_rate); }

	public: /* virtual */

		virtual void setup(void);
		virtual void initialize(void);
		virtual void finalize(void);

		virtual void render(void);
		virtual void second(void);
		virtual void tick(TIME &timeout);
		virtual void update(TIME &timeout);

	public: /* static */

		static Engine * Instance(void)
		    { return(s_instance); }
	};

}

MARSHMALLOW_NAMESPACE_END

#endif
