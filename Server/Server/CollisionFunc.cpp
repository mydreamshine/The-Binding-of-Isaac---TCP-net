#include "GameProcessFunc.h"

bool CollisionFunc::IntersectRect(fRECT* intersectRect, fRECT& Rect1, fRECT& Rect2)
{
	bool VCollision = false;
    bool HCollision = false;

    // 수평 충돌
	if (Rect1.Left < Rect2.Right && Rect1.Right > Rect2.Left)
	{
		HCollision = true;
		intersectRect->Left = max(Rect1.Left, Rect2.Left);
		intersectRect->Right = min(Rect1.Right, Rect2.Right);
	}
    // 수직 충돌
	if (Rect1.Top > Rect2.Bottom && Rect1.Bottom < Rect2.Top)
	{
		VCollision = true;
		intersectRect->Top = min(Rect1.Top, Rect2.Top);
		intersectRect->Bottom = max(Rect1.Bottom, Rect2.Bottom);
	}
	return VCollision && HCollision;
}

bool CollisionFunc::CollideWndBoundary(fRECT& ObjRect, fRECT& WndBoundary)
{
	if (ObjRect.Left < WndBoundary.Left)     return true;
	if (ObjRect.Top > WndBoundary.Top)       return true;
	if (ObjRect.Right > WndBoundary.Right)   return true;
	if (ObjRect.Bottom < WndBoundary.Bottom) return true;
	return false;
}
