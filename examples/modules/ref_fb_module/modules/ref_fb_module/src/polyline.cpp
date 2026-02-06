#include <ref_fb_module/polyline.h>
#include <ref_fb_module/polyline_impl_generic.h>
#include <ref_fb_module/polyline_impl_line.h>

Polyline::Polyline(float thickness, LineStyle style)
{
    if (thickness == 1.0f)
        pImpl = std::make_unique<PolylineImplLine>(style);
    else
        pImpl = std::make_unique<PolylineImplGeneric>(thickness, style);
        
}

void Polyline::addPoint(const sf::Vector2f& pt)
{
    pImpl->addPoint(pt);
}

void Polyline::addPoint(float x, float y)
{
    addPoint(sf::Vector2f(x, y));
}

void Polyline::close()
{
    pImpl->close();
}

void Polyline::setColor(const sf::Color& color)
{
    pImpl->setColor(color);
}

void Polyline::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    states.texture = nullptr;

    target.draw(*pImpl, states);
}
