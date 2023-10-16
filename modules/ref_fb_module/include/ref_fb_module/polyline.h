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
#include <memory>

class Polyline : public sf::Drawable, public sf::Transformable
{
public:
    explicit Polyline(float thickness = 10.0f, LineStyle style = LineStyle::solid);
    void addPoint(const sf::Vector2f& pt);
    void addPoint(float x, float y);
    void close();
    void setColor(const sf::Color& color);
protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
private:

    std::unique_ptr<PolylineImpl> pImpl;
};
