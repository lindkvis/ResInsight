/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018-     Statoil ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cvfBase.h"
#include "cvfDrawableGeo.h"
#include "cvfObject.h"
#include "cvfVector3.h"

#include "cafPdmPointer.h"

#include <vector>

namespace caf
{
class DisplayCoordTransform;
}

namespace cvf
{
class BoundingBox;
}

class RigWellPath;
class RimWellPath;
class Rim3dWellLogCurve;

class Riv3dWellLogCurveGeometryGenerator : public cvf::Object
{
public:
    typedef std::pair<cvf::Vec3d, double> PointValuePair;
    Riv3dWellLogCurveGeometryGenerator(RimWellPath* wellPath);

    void createCurveDrawables(const caf::DisplayCoordTransform* displayCoordTransform,
                              const cvf::BoundingBox&           wellPathClipBoundingBox,
                              const Rim3dWellLogCurve*          rim3dWellLogCurve,
                              double                            planeOffsetFromWellPathCenter,
                              double                            planeWidth,
                              const std::vector<cvf::Vec3f>&    drawSurfaceVertices);

    void clearCurvePointsAndGeometry();

    const RigWellPath* wellPathGeometry() const;

    cvf::ref<cvf::DrawableGeo> curveDrawable();
    cvf::ref<cvf::DrawableGeo> curveFilledDrawable();

    bool findClosestPointOnCurve(const cvf::Vec3d& globalIntersection,
                                 cvf::Vec3d*       closestPoint,
                                 double*           measuredDepthAtPoint,
                                 double*           valueAtClosestPoint) const;

private:
    void        createNewVerticesAlongTriangleEdges(const std::vector<cvf::Vec3f>& drawSurfaceVertices);
    static void createNewVerticesAlongSegment(const cvf::Vec3f&              ptStart,
                                              const cvf::Vec3f&              ptEnd,
                                              const std::vector<cvf::Vec3f>& drawSurfaceVertices,
                                              std::vector<cvf::Vec3f>*       extraVertices,
                                              const cvf::Vec3f*              ptBottomStart = nullptr,
                                              const cvf::Vec3f*              ptBottomEnd = nullptr,
                                              std::vector<cvf::Vec3f>*       extraBottomVertices = nullptr);

    static std::vector<cvf::Vec3f> projectVerticesOntoTriangles(const std::vector<cvf::Vec3f>& originalVertices, const std::vector<cvf::Vec3f>& drawSurfaceVertices);
    static cvf::Vec3f              projectPointOntoTriangle(const cvf::Vec3f& point,
                                                            const cvf::Vec3f& triangleVertex1,
                                                            const cvf::Vec3f& triangleVertex2,
                                                            const cvf::Vec3f& triangleVertex3,
                                                            bool*             wasInsideTriangle);
    caf::PdmPointer<RimWellPath> m_wellPath;
    double                       m_planeWidth;
	
    cvf::ref<cvf::DrawableGeo>   m_curveDrawable;
    cvf::ref<cvf::DrawableGeo>   m_curveFilledDrawable;
    std::vector<cvf::Vec3f>      m_curveVertices;
    std::vector<cvf::Vec3f>      m_bottomVertices;

    std::vector<double>          m_curveMeasuredDepths;
    std::vector<double>          m_curveValues;
};
