#include <ref_fb_module/polyline_impl_generic.h>
#include <cmath>

PolylineImplGeneric::PolylineImplGeneric(float thickness, LineStyle style)
    : PolylineImpl(sf::PrimitiveType::Triangles, style)
    , m_hasStartPt(false)
    , m_havePrevRightPts(false)
    , m_haveStartLeftPts(false)
    , m_closed(false)
    , m_thickness(thickness)
    , m_style(style)
{
}

void PolylineImplGeneric::addPoint(const sf::Vector2f& pt)
{
    if (m_closed)
        throw std::runtime_error("line closed");

    if (!m_hasStartPt)
    {
        m_startPt = pt;
        m_prevPt = pt;
        m_hasStartPt = true;
    }
    else
    {
        auto vec = pt - m_prevPt;
        auto normVec = normalize(vec);
        auto halfOrthoNormVec = orthogonal(normVec) * m_thickness / 2.0f;

        sf::Vector2f topLeft, bottomLeft, topRight, bottomRight;
        if (m_style == LineStyle::solid)
        {
            calcRects(m_prevPt, pt, halfOrthoNormVec, topLeft, bottomLeft, topRight, bottomRight);
            drawLine(topLeft, bottomLeft, topRight, bottomRight);

            if (!m_haveStartLeftPts)
            {
                m_startTopLeft = topLeft;
                m_startBottomLeft = bottomLeft;
                m_haveStartLeftPts = true;
            }

            if (m_havePrevRightPts)
            {
                drawTriangle(m_prevTopRight, topLeft, m_prevPt);
                drawTriangle(m_prevPt, bottomLeft, m_prevBottomRight);
            }

            m_prevPt = pt;
        }
        else
        {
            auto drawDistance = m_thickness * 5.0f;
            auto skipDistance = m_thickness * 5.0f;
            auto distance = getDistance(vec);
            auto drawnDistance = 0.0f;
            while (drawnDistance < distance)
            {
                drawnDistance += drawDistance;
                sf::Vector2f nextPt;
                if (drawnDistance < distance)
                    nextPt = m_prevPt + normVec * drawDistance;
                else
                    nextPt = pt;
                calcRects(m_prevPt, nextPt, halfOrthoNormVec, topLeft, bottomLeft, topRight, bottomRight);
                drawLine(topLeft, bottomLeft, topRight, bottomRight);

                if (!m_haveStartLeftPts)
                {
                    m_startTopLeft = topLeft;
                    m_startBottomLeft = bottomLeft;
                    m_haveStartLeftPts = true;
                }
                
                m_prevPt = nextPt + normVec * skipDistance;
                drawnDistance += skipDistance;
            }

/*            if (m_havePrevRightPts)
            {
                drawTriangle(m_prevTopRight, topLeft, m_prevPt);
                drawTriangle(m_prevPt, bottomLeft, m_prevBottomRight);
            }*/

            m_prevPt = pt;
        }

        m_havePrevRightPts = true;
        m_prevTopRight = topRight;
        m_prevBottomRight = bottomRight;
    }
}

void PolylineImplGeneric::close()
{
    if (m_closed)
        throw std::runtime_error("line closed");
    if (!m_hasStartPt)
        throw std::runtime_error("no start point defined");

    addPoint(m_startPt);

    drawTriangle(m_prevTopRight, m_startTopLeft, m_prevPt);
    drawTriangle(m_prevPt, m_startBottomLeft, m_prevBottomRight);

    m_closed = true;
}

void PolylineImplGeneric::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.texture = nullptr;
    target.draw(m_vertices, states);
}

void PolylineImplGeneric::drawLine(const sf::Vector2f& topLeft, const sf::Vector2f& bottomLeft, const sf::Vector2f& topRight, const sf::Vector2f& bottomRight)
{
    drawTriangle(bottomLeft, topLeft, topRight);
    drawTriangle(topRight, bottomRight, bottomLeft);
}

void PolylineImplGeneric::drawTriangle(const sf::Vector2f& pt1, const sf::Vector2f& pt2, const sf::Vector2f& pt3)
{
    m_vertices.append(sf::Vertex(pt1, m_color));
    m_vertices.append(sf::Vertex(pt2, m_color));
    m_vertices.append(sf::Vertex(pt3, m_color));
}

void PolylineImplGeneric::calcRects(const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& halfOrthoNormVec, sf::Vector2f& topLeft, sf::Vector2f& bottomLeft, sf::Vector2f& topRight, sf::Vector2f& bottomRight)
{
    bottomLeft = p1 + halfOrthoNormVec;
    topLeft = p1 - halfOrthoNormVec;
    bottomRight = p2 + halfOrthoNormVec;
    topRight = p2 - halfOrthoNormVec;
}
