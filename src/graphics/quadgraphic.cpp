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

#include "graphics/quadgraphic.h"

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#include <tinyxml.h>

#include "graphics/textureasset.h"

MARSHMALLOW_NAMESPACE_USE;
using namespace Graphics;

const Core::Type QuadGraphic::Type("Graphics::QuadGraphic");

QuadGraphic::QuadGraphic(const Math::Vector2 &tl,
                         const Math::Vector2 &bl,
                         const Math::Vector2 &br,
                         const Math::Vector2 &tr)
    : GraphicBase()
{
	m_vectors[0] = tl;
	m_vectors[1] = bl;
	m_vectors[2] = br;
	m_vectors[3] = tr;
}

QuadGraphic::QuadGraphic(const Math::Rect2  &r)
    : GraphicBase()
{
	m_vectors[0] = r.topLeft();
	m_vectors[1] = r.bottomLeft();
	m_vectors[2] = r.bottomRight();
	m_vectors[3] = r.topRight();
}

QuadGraphic::QuadGraphic(void)
    : GraphicBase()
{
	m_vectors[0] = Math::Vector2::Null;
	m_vectors[1] = Math::Vector2::Null;
	m_vectors[2] = Math::Vector2::Null;
	m_vectors[3] = Math::Vector2::Null;
}

QuadGraphic::~QuadGraphic(void)
{
}

bool
QuadGraphic::serialize(TinyXML::TiXmlElement &n) const
{
	return(GraphicBase::serialize(n));
}

bool
QuadGraphic::deserialize(TinyXML::TiXmlElement &n)
{

	if (!GraphicBase::deserialize(n))
		return(false);

	int l_i;
	TinyXML::TiXmlElement *l_child;
	for (l_i = 0, l_child = n.FirstChildElement("vector");
	     l_i < 4 && l_child;
	     ++l_i, l_child = l_child->NextSiblingElement("vector")) {
		l_child->QueryFloatAttribute("x", &m_vectors[l_i].rx());
		l_child->QueryFloatAttribute("y", &m_vectors[l_i].ry());
	}

	return(true);
}
