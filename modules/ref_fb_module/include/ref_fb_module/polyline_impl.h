/*
 * Copyright 2022-2024 openDAQ d. o. o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4242)
#endif

#include <SFML/Graphics.hpp>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

enum class LineStyle
{
    solid,
    dash
};

class PolylineImpl : public sf::Drawable
{
public:
    explicit PolylineImpl(sf::PrimitiveType primitiveType, LineStyle style = LineStyle::solid);
    virtual void addPoint(const sf::Vector2f& pt) = 0;
    virtual void close() = 0;
    void setColor(const sf::Color& color);

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    static sf::Vector2f normalize(const sf::Vector2f& v);
    static sf::Vector2f orthogonal(const sf::Vector2f& v);
    static float getDistance(const sf::Vector2f& v);
    static float dotProduct(const sf::Vector2f& v1, const sf::Vector2f& v2);

    void updateColors();

    sf::VertexArray m_vertices;
    sf::Color m_color;
    LineStyle m_style;
};
