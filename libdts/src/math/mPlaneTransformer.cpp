//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "math/mPlaneTransformer.h"
#include "math/mMathFn.h"

//-----------------------------------------------------------------------------

BEGIN_NS(DTShape)

//-----------------------------------------------------------------------------

void PlaneTransformer::set(const MatrixF& xform, const Point3F& scale)
{
   mTransform = xform;
   mScale     = scale;

   MatrixF scaleMat(true);
   F32* m = scaleMat;
   m[MatrixF::idx(0, 0)] = scale.x;
   m[MatrixF::idx(1, 1)] = scale.y;
   m[MatrixF::idx(2, 2)] = scale.z;

   mTransposeInverse = xform;
   mTransposeInverse.mul(scaleMat);
   mTransposeInverse.transpose();
   mTransposeInverse.inverse();
}

void PlaneTransformer::transform(const PlaneF& plane, PlaneF& result)
{
   Point3F point = plane;
   point *= -plane.d;
   point.convolve(mScale);
   mTransform.mulP(point);

   Point3F normal = plane;
   mTransposeInverse.mulV(normal);

   result.set(point, normal);
//   mTransformPlane(mTransform, mScale, plane, &result);
}

void PlaneTransformer::setIdentity()
{
   static struct MakeIdentity
   {
      PlaneTransformer  transformer;
      MakeIdentity()
      {
         MatrixF  defMat(true);
         Point3F  defScale(1.0f, 1.0f, 1.0f);
         transformer.set(defMat, defScale);
      }
   } sMakeIdentity;

   *this = sMakeIdentity.transformer;
}

//-----------------------------------------------------------------------------

END_NS
