#include <ref_fb_module/polyline_impl.h>
#include <cmath>

PolylineImpl::PolylineImpl(sf::PrimitiveType primitiveType, LineStyle style)
    : m_vertices(primitiveType)
    , m_color(sf::Color::White)
    , m_style(style)
{
}

void PolylineImpl::setColor(const sf::Color& color)
{
    m_color = color;
    updateColors();
}

void PolylineImpl::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.texture = nullptr;
    target.draw(m_vertices, states);
}

void PolylineImpl::updateColors()
{
    for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i)
        m_vertices[i].color = m_color;
}

float PolylineImpl::dotProduct(const sf::Vector2f& v1, const sf::Vector2f& v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

float PolylineImpl::getDistance(const sf::Vector2f& v)
{
    return std::sqrt(dotProduct(v, v));
}

sf::Vector2f PolylineImpl::normalize(const sf::Vector2f& v)
{
    float dist = getDistance(v);

    return v / dist;
}

sf::Vector2f PolylineImpl::orthogonal(const sf::Vector2f& v)
{
    return sf::Vector2f(-v.y, v.x);
}
