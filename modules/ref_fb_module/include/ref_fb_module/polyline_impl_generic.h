/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include <ref_fb_module/polyline_impl.h>

class PolylineImplGeneric : public PolylineImpl
{
public:
    explicit PolylineImplGeneric(float thickness = 10.0f, LineStyle style = LineStyle::solid);
    void addPoint(const sf::Vector2f& pt) override;
    void close() override;
protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
private:
    void drawLine(const sf::Vector2f& topLeft, const sf::Vector2f& bottomLeft, const sf::Vector2f& topRight, const sf::Vector2f& bottomRight);
    void drawTriangle(const sf::Vector2f& pt1, const sf::Vector2f& pt2, const sf::Vector2f& pt3);
    void calcRects(const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& halfOrthoNormVec, sf::Vector2f& topLeft, sf::Vector2f& bottomLeft, sf::Vector2f& topRight, sf::Vector2f& bottomRight);

    sf::Vector2f m_startPt;
    sf::Vector2f m_prevPt;
    sf::Vector2f m_prevTopRight;
    sf::Vector2f m_prevBottomRight;
    sf::Vector2f m_startTopLeft;
    sf::Vector2f m_startBottomLeft;
    bool m_hasStartPt;
    bool m_havePrevRightPts;
    bool m_haveStartLeftPts;
    bool m_closed;
    float m_thickness;
    LineStyle m_style;
};
