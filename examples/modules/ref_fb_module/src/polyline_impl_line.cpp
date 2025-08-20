#include <ref_fb_module/polyline_impl_line.h>

PolylineImplLine::PolylineImplLine(LineStyle style)
    : PolylineImpl(style == LineStyle::solid ? sf::PrimitiveType::LinesStrip : sf::PrimitiveType::Lines, style)
{
}

void PolylineImplLine::addPoint(const sf::Vector2f& pt)
{
    if (m_style == LineStyle::solid)
    {
        m_vertices.append(sf::Vertex(pt, m_color));
    }
    else
    {
        if (!m_vertices.getVertexCount())
        {
            m_vertices.append(sf::Vertex(pt, m_color));
            m_vertices.append(sf::Vertex(pt, m_color));
            return;
        }
        auto prevPt = m_vertices[m_vertices.getVertexCount() - 1].position;
        auto vec = pt - prevPt;
        auto normVec = normalize(vec);

        const float thickness = 1.0f;
        auto drawDistance = thickness * 5.0f;
        auto skipDistance = thickness * 5.0f;
        auto distance = getDistance(vec);
        auto drawnDistance = 0.0f;
        while (drawnDistance < distance)
        {
            drawnDistance += drawDistance;
            sf::Vector2f nextPt;
            if (drawnDistance < distance)
                nextPt = prevPt + normVec * drawDistance;
            else
                nextPt = pt;

            m_vertices.append(sf::Vertex(prevPt, m_color));
            m_vertices.append(sf::Vertex(nextPt, m_color));

            prevPt = nextPt + normVec * skipDistance;
            drawnDistance += skipDistance;
        }
    }
}

void PolylineImplLine::close()
{
    if (!m_vertices.getVertexCount())
        throw std::runtime_error("no start point defined");

    addPoint(m_vertices[0].position);
}
