//
// V2 vector type
//

internal inline v2 operator+(v2 A, v2 B)
{
	return v2{ A.X + B.X, A.Y + B.Y };
}

internal inline v2 operator-(v2 A, v2 B)
{
	return v2{ A.X - B.X, A.Y - B.Y };
}

internal inline v2 operator-(v2 A)
{
	return v2{ -A.X, -A.Y };
}

internal inline v2 operator*(v2 A, v2 B)
{
	return v2{ A.X * B.X, A.Y * B.Y };
}

internal inline v2 operator*(f32 A, v2 B)
{
	return v2{ A * B.X, A * B.Y };
}

internal inline v2 operator/(v2 A, v2 B)
{
	return v2{ A.X / B.X, A.Y / B.Y };
}

internal inline v2 operator/(v2 A, f32 B)
{
	return v2{ A.X / B, A.Y / B };
}

internal inline v2 operator/(f32 A, v2 B)
{
	return v2{ A / B.X, A / B.Y };
}

internal inline v2 &operator+=(v2 &A, v2 B)
{
	A = A + B;
	
	return A;
}

internal inline v2 &operator-=(v2 &A, v2 B)
{
	A = A - B;
	
	return A;
}

internal inline v2 &operator*=(v2 &A, f32 B)
{
	A = B * A;
	
	return A;
}

internal inline v2 &operator*=(v2 &A, v2 B)
{
	A = B * A;
	
	return A;
}

//
// @Warn floor(7.999) gives 8 which is dangerous
// It happens because of rounding during the fp-add
//

internal inline f32 DotV2(v2 A, v2 B)
{
	f32 Result = A.X * B.X + A.Y * B.Y;
	
	return Result;
}

internal inline f32 DistanceBetweenTwoPointsSq(v2 A, v2 B)
{
	f32 Result = Square(B.X - A.X) + Square(B.Y - A.Y);
	
	return Result;
}

internal inline bmm PointInRectangle(v2 Position, v4 Rectangle)
{
	bmm Result = ((Position.X >= Rectangle.Left)  && (Position.Y >= Rectangle.Bottom) && 
				  (Position.X <  Rectangle.Right) && (Position.Y <  Rectangle.Top));
	
	return Result;
}

internal inline bmm PointInRange(f32 Value, f32 Start, f32 End)
{
	bmm Result = ((Value >= Start) && 
				  (Value <  End));
	
	return Result;
}

internal inline bmm LineInRange(f32 LineStart, f32 LineEnd, f32 RangeStart, f32 RangeEnd)
{
	bmm Result = ((LineEnd   >= RangeStart) && 
				  (LineStart <  RangeEnd));
	
	return Result;
}

internal inline s32 FloorF32(f32 Value)
{
	s32 Result = (s32)(Value + 32768.0f) - 32768;
	
	return Result;
}

internal inline s32 CeilF32(f32 Value)
{
	s32 Result = 32768 - (s32)(32768.0f - Value);
	
	return Result;
}
