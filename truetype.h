//
// Truetype
//

union glyph_edge
{
	struct
	{
		f32 X1;
		f32 Y1;
		f32 X2;
		f32 Y2;
	};
	
	struct
	{
		v2 ThisPoint;
		v2 PrevPoint;
	};
};

struct glyph_active_edge
{
	f32 FX;
	f32 FDeltaX;
	f32 FDeltaY;
	f32 Direction;
	f32 SY;
	f32 EY;
};

#define Read8()  (u8)	  (*(u8  *)(READ_Data + READ_Offset)); READ_Offset += sizeof(u8);  Assert(READ_Offset <= READ_Size);
#define Read16() (u16)BE16(*(u16 *)(READ_Data + READ_Offset)); READ_Offset += sizeof(u16); Assert(READ_Offset <= READ_Size);
#define Read32() (u32)BE32(*(u32 *)(READ_Data + READ_Offset)); READ_Offset += sizeof(u32); Assert(READ_Offset <= READ_Size);
#define Read64() (u64)BE64(*(u64 *)(READ_Data + READ_Offset)); READ_Offset += sizeof(u64); Assert(READ_Offset <= READ_Size);

#define SIMPLE_GLYPH_FLAG_ON_CURVE_POINT						0x01
#define SIMPLE_GLYPH_FLAG_X_SHORT_VECTOR						0x02
#define SIMPLE_GLYPH_FLAG_Y_SHORT_VECTOR						0x04
#define SIMPLE_GLYPH_FLAG_REPEAT_FLAG							0x08
#define SIMPLE_GLYPH_FLAG_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR	0x10
#define SIMPLE_GLYPH_FLAG_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR	0x20
#define SIMPLE_GLYPH_FLAG_OVERLAP_SIMPLE						0x40
#define SIMPLE_GLYPH_FLAG_RESERVED								0x80 // Must be set to 0

internal inline void TTFGlyfParseCoords(u8 *READ_Data, smm *OffsetIn, smm READ_Size, u8 *Flags, smm CoordCount, s16 *Coords, smm CoordIndex)
{
	// Get the current file read offset
	// Can't be a pointer for the Read8,16,32,64 macros to work
	smm READ_Offset = *OffsetIn;
	
	// CoordIndex is 0 for X, and 1 for Y
	
	// The first coordinate is relative to (0, 0)
	// and the others are relative to the previous one
	s16 LastCoord = 0;
	for (smm Index = 0; Index < CoordCount; ++Index)
	{
		smm IsSameOrPositive	= ((Flags[Index] & (SIMPLE_GLYPH_FLAG_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR << CoordIndex)) != 0);
		smm IsShortVector		= ((Flags[Index] & (SIMPLE_GLYPH_FLAG_X_SHORT_VECTOR << CoordIndex)) != 0);
		if (IsShortVector)
		{
			smm IsPositive = IsSameOrPositive;
			
			s16 RawCoord = (s16)Read8();
			Coords[Index] = LastCoord + RawCoord * (IsPositive ? +1 : -1);
		}
		else
		{
			smm IsTheSameAsLast = IsSameOrPositive;
			if (IsTheSameAsLast)
			{
				Coords[Index] = LastCoord;
			}
			else
			{
				Coords[Index] = LastCoord + (s16)Read16();
			}
		}
		
		LastCoord = Coords[Index];
	}
	
	// Write the new file read offset to the main function
	*OffsetIn = READ_Offset;
}

internal void HandleClippedEdge(
	f32 *ScanLine, smm X, glyph_active_edge *Edge, f32 X0, f32 Y0, f32 X1, f32 Y1)
{
	if (Y0 == Y1) return;
	Assert(Y0 < Y1);
	Assert(Edge->SY <= Edge->EY);
	
	if (Y0 > Edge->EY) return;
	if (Y1 < Edge->SY) return;
	
	f32 FX = (f32)X;
	
	if (Y0 < Edge->SY)
	{
		X0 += (X1 - X0) * (Edge->SY - Y0) / (Y1 - Y0);
		Y0 = Edge->SY;
	}
	if (Y1 > Edge->EY)
	{
		X1 += (X1 - X0) * (Edge->EY - Y1) / (Y1 - Y0);
		Y1 = Edge->EY;
	}
	
	if (X0 == FX)
	{
		Assert(X1 <= (FX + 1));
	}
	else if (X0 == (FX + 1))
	{
		Assert(X1 >= FX);
	}
	else if (X0 <= FX)
	{
		Assert(X1 <= FX);
	}
	else if (X0 >= (FX + 1.0f))
	{
		Assert(X1 >= (FX + 1.0f));
	}
	else
	{
		Assert((X1 >= FX) && (X1 <= (FX + 1.0f)));
	}
	
	if ((X0 <= FX) && (X1 <= FX))
	{
		ScanLine[X] += Edge->Direction * (Y1 - Y0);
	}
	else if ((X0 >= (FX + 1.0f)) && (X1 >= (FX + 1.0f)))
	{
		// Nothing
	}
	else
	{
		Assert((X0 >= FX) && (X0 <= (FX + 1)) && (X1 >= FX) && (X1 <= (FX + 1)));
		
		// Coverage = 1 - Average X Position
		ScanLine[X] += Edge->Direction * (Y1 - Y0) * ((1.0f - ((X0 - FX) + (X1 - FX)) * 0.5f));
	}
}

internal inline void MakeEdge(v2 ThisPoint, v2 PrevPoint, 
	smm *EdgeCount, smm MaxEdgeCount, glyph_edge *Edges, u8 *EdgesYSwap)
{
	// Cull horizontal edges, because they are not needed
	// scanlines are horizontal and it will not have any effect
	if (PrevPoint.Y != ThisPoint.Y)
	{
		// Swap P0 and P1 so that P0 ALWAYS has higher Y than P1
		// This is useful for sorting and rasterization below
		// because we can guarantee which Y is bigger
		bmm DoSwap = (ThisPoint.Y > PrevPoint.Y);
		if (DoSwap)
		{
			v2 SwapTemp = ThisPoint;
			ThisPoint = PrevPoint;
			PrevPoint = SwapTemp;
		}
		
		Assert(*EdgeCount < MaxEdgeCount);
		
		EdgesYSwap[*EdgeCount] = DoSwap;
		Edges[*EdgeCount] = glyph_edge{ 
			ThisPoint.X, ThisPoint.Y,
			PrevPoint.X, PrevPoint.Y
		};
		
		*EdgeCount += 1;
	}
}

internal smm RasterizeGlyph(smm GlyphIndex)
{
	font *Font = &GLOBAL_MainState->Font;
	
	if (GlyphIndex < 0) { GlyphIndex = 0; }
	Assert(GlyphIndex < Font->GlyphCount);
	
	//
	// Check if the glyph is already rasterized
	//
	
	for (smm LoadedGlyph = 0; 
		LoadedGlyph < Font->LoadedGlyphCount; 
		++LoadedGlyph)
	{
		if (GlyphIndex == Font->LoadedGlyphsCodepoint[LoadedGlyph])
		{
			// The glyph is already rasterized
			Font->LoadedGlyphsUsedFrame[LoadedGlyph] = GLOBAL_MainState->Frame;
			return LoadedGlyph;
		}
	}
	
	// @Note We are not modifying the actual arena
	// This is a local copy that SHOULD NOT affect the TransientArena
	memory_arena ArenaSnapshot = GLOBAL_MainState->TransientArena;
	memory_arena *TempArena = &ArenaSnapshot;
	
	//
	// GLYF
	//
	
	u8 *READ_Data  = Font->FileData;
	smm READ_Size = Font->FileSize;
	
	smm READ_Offset;
	smm OffsetToGlyph;
	if (Font->IndexToLocFormat == 1)
	{
		READ_Offset		= Font->LocaOffset + GlyphIndex * sizeof(u32);
		OffsetToGlyph	= Read32();
	}
	else if (Font->IndexToLocFormat == 0)
	{
		READ_Offset		= Font->LocaOffset + GlyphIndex * sizeof(u16);
		OffsetToGlyph	= 2 * (smm)Read16();
	}
	else
	{
		OffsetToGlyph = 0;
		Assert(false);
	}
	
	READ_Offset = Font->GlyfOffset + OffsetToGlyph;
	
	smm GLYF_NumberOfContours	= (s16)Read16();
	smm GLYF_XMin				= (s16)Read16();
	smm GLYF_YMin				= (s16)Read16();
	smm GLYF_XMax				= (s16)Read16();
	smm GLYF_YMax				= (s16)Read16();
	
	Print("FontSize %f", Font->FontSize);
	Print("ScaleFactor %f", Font->ScaleFactor);
	Print("GLYF_NumberOfContours %d", GLYF_NumberOfContours);
	
	Print("GLYF_XMin %d", GLYF_XMin);
	Print("GLYF_XMax %d", GLYF_XMax);
	Print("GLYF_YMin %d", GLYF_YMin);
	Print("GLYF_YMax %d", GLYF_YMax);
	
	if (GLYF_NumberOfContours == -1) // COMPOSITE glyph
	{
		return 0;
	}
	
	u16 *EndPtsOfContours = ArenaAlloc(TempArena, u16, GLYF_NumberOfContours);
	for (smm ContourIndex = 0; ContourIndex < GLYF_NumberOfContours; ++ContourIndex)
	{
		EndPtsOfContours[ContourIndex] = Read16();
	}
	
	smm InstructionLength = Read16();
	if (InstructionLength > 0)
	{
		Print("InstructionLength %d", InstructionLength);
		u8 *Instructions = ArenaAlloc(TempArena, u8, InstructionLength);
		for (smm Index = 0; Index < InstructionLength; ++Index)
		{
			Instructions[Index] = Read8();
		}
	}
	
	smm LastPointIndex = EndPtsOfContours[GLYF_NumberOfContours - 1];
	smm PointCount = LastPointIndex + 1;
	
	Print("PointCount %d", PointCount);
	
	u8 *Flags = ArenaAlloc(TempArena, u8, PointCount);
	for (smm Index = 0; Index < PointCount; ++Index)
	{
		Flags[Index] = Read8();
		
		smm IsRepeated = ((Flags[Index] & SIMPLE_GLYPH_FLAG_REPEAT_FLAG) != 0);
		if (IsRepeated)
		{
			smm RepeatCount = Read8();
			Print("RepeatCount %d", RepeatCount);
			
			u8 FlagToRepeat = Flags[Index];
			for (smm RepeatIndex = 0; RepeatIndex < RepeatCount; ++RepeatIndex)
			{
				Assert((Index + 1) <= PointCount);
				Flags[++Index] = FlagToRepeat;
			}
		}
	}
	
	s16 *PointsX = ArenaAlloc(TempArena, s16, PointCount);
	s16 *PointsY = ArenaAlloc(TempArena, s16, PointCount);
	
	TTFGlyfParseCoords(READ_Data, &READ_Offset, READ_Size, Flags, PointCount, PointsX, 0); // X
	TTFGlyfParseCoords(READ_Data, &READ_Offset, READ_Size, Flags, PointCount, PointsY, 1); // Y
	
	//
	// Scale points
	//
	
	f32 Scale = Font->FontSize * Font->ScaleFactor;
	v2 *Points = ArenaAlloc(TempArena, v2, PointCount);
	for (smm Index = 0; Index < PointCount; ++Index)
	{
		Points[Index] = v2{ (f32)PointsX[Index], (f32)PointsY[Index] } * v2{ Scale, -Scale };
	}
	
	//
	// Gather all edges
	//
	
	smm MaxEdgeCount = PointCount * 2;
	smm EdgeCount = 0;
	glyph_edge *Edges		= ArenaAlloc(TempArena, glyph_edge,	MaxEdgeCount);
	u8		   *EdgesYSwap	= ArenaAlloc(TempArena, u8, 		MaxEdgeCount);
	{
		v2 PrevPoint;
		smm PointIndex = 0;
		for (smm ContourIndex = 0; 
			ContourIndex < GLYF_NumberOfContours; 
			++ContourIndex)
		{
			smm FirstInContour = PointIndex;
			
			PrevPoint = Points[PointIndex];
			++PointIndex;
			
			for (; PointIndex <= EndPtsOfContours[ContourIndex]; )
			{
				v2 ThisPoint = Points[PointIndex];
				
				smm IsOnCurve = ((Flags[PointIndex] & SIMPLE_GLYPH_FLAG_ON_CURVE_POINT) != 0);
				if (IsOnCurve)
				{
					MakeEdge(ThisPoint, PrevPoint, 
						&EdgeCount, MaxEdgeCount, Edges, EdgesYSwap);
				}
				else
				{
					//
					// @Todo TO DO DEBUG ------------ THERE IS A MISTAKE IN THE BEZIER INTERPOLATION MAKING EDGES
					// @Debug
					//
					
					#if 0
					v2 NextPoint = Points[PointIndex + 1];
					smm IsNextOnCurve = ((Flags[PointIndex + 1] & SIMPLE_GLYPH_FLAG_ON_CURVE_POINT) != 0);
					if (!IsNextOnCurve)
					{
						// Split the CUBIC curve into TWO QUADRATIC curves
						// This is the midpoint we use to do the split
						NextPoint = ThisPoint + 0.5f * (NextPoint - ThisPoint);
					}
					
					//
					// Tesselate bezier
					//
					
					smm LastIndex = EdgeCount - 1;
					v2 LastEdgePoint = (EdgesYSwap[LastIndex] ? 
						Edges[LastIndex].PrevPoint : Edges[LastIndex].ThisPoint);
					
					smm BezierIterations = 1;
					for (smm Index = 0; Index <= BezierIterations; ++Index)
					{
						f32 T = (f32)Index * (1.0f / (f32)BezierIterations);
						
						v2 Point = 
							Square(1.0f - T) * PrevPoint + 
							2.0f * T * (1.0f - T) * ThisPoint + 
							Square(T) * NextPoint;
						
						MakeEdge(Point, LastEdgePoint, 
							&EdgeCount, MaxEdgeCount, Edges, EdgesYSwap);
						LastEdgePoint = Point;
					}
					ThisPoint = PrevPoint;
					#else
					MakeEdge(ThisPoint, PrevPoint, 
						&EdgeCount, MaxEdgeCount, Edges, EdgesYSwap);
					#endif
				}
				
				// Next point
				PrevPoint = ThisPoint;
				++PointIndex;
			}
			
			// Close the contours
			MakeEdge(Points[FirstInContour], PrevPoint, 
				&EdgeCount, MaxEdgeCount, Edges, EdgesYSwap);
		}
	}
	
	//
	// Debug print of all edge points
	//
	
	Print("EdgeCount %d", EdgeCount);
	for (smm Index = 0; Index < EdgeCount; ++Index)
	{
		Print("%f, %f\n%f, %f\n", 
			Edges[Index].X1, Edges[Index].Y1, 
			Edges[Index].X2, Edges[Index].Y2);
	}
	
	//
	// Insertion Sort for Glyph Edges
	// @Todo Time it and choose a better algorithm
	//
	
	for (smm SortIndex = 1; SortIndex < EdgeCount; ++SortIndex)
	{
		glyph_edge ThisEdge = Edges[SortIndex];
		b8 ThisYSwap = EdgesYSwap[SortIndex];
		
		// Y1 is always higher than Y2
		// because we swap them inside MakeEdge
		smm J = SortIndex - 1;
		for (; (J >= 0) && (Edges[J].Y1 > ThisEdge.Y1); --J)
		{
			Edges[J + 1]	  = Edges[J];
			EdgesYSwap[J + 1] = EdgesYSwap[J];
		}
		
		Edges[J + 1]	  = ThisEdge;
		EdgesYSwap[J + 1] = ThisYSwap;
	}
	
	//
	// Glyph Bounding Box
	//
	
	smm BBoxX1 = FloorF32(GLYF_XMin * +Scale);
	smm BBoxY1 = FloorF32(GLYF_YMax * -Scale);
	
	smm BBoxX2 = CeilF32(GLYF_XMax * +Scale);
	smm BBoxY2 = CeilF32(GLYF_YMin * -Scale);
	
	smm BBoxWidth	= BBoxX2 - BBoxX1;
	smm BBoxHeight	= BBoxY2 - BBoxY1;
	
	Print("BBoxX1 %d", BBoxX1);
	Print("BBoxX2 %d", BBoxX2);
	Print("BBoxY1 %d", BBoxY1);
	Print("BBoxY2 %d", BBoxY2);
	
	Print("BBoxWidth %d", BBoxWidth);
	Print("BBoxHeight %d", BBoxHeight);
	
	//
	// Glyph Rasterization
	//
	
	glyph_active_edge *ActiveEdges = ArenaAlloc(TempArena, glyph_active_edge, EdgeCount);
	smm ActiveEdgeCount = 0;
	
	#define FONT_CELL_WIDTH		64
	#define FONT_CELL_HEIGHT	64
	
	// Clear the outputing image to black
	u8 *OutputImageU8 = ArenaAlloc(TempArena, u8, FONT_CELL_WIDTH * FONT_CELL_HEIGHT);
	
	smm EdgeIndex = 0; // Continous edge index
	f32 ScanlineY = (f32)BBoxY1;
	for (smm ScanlineIndex = 0; ScanlineIndex < BBoxHeight; ++ScanlineIndex)
	{
		f32 ScanlineTop		= ScanlineY + 0.0f;
		f32 ScanlineBottom	= ScanlineY + 1.0f;
		
		// Remove active edges that terminate before the top of the scanline
		for (smm ActiveEdgeIndex = ActiveEdgeCount - 1; ActiveEdgeIndex >= 0; --ActiveEdgeIndex)
		{
			if (ActiveEdges[ActiveEdgeIndex].EY <= ScanlineTop)
			{
				// Delete from array
				for (smm ShiftIndex = ActiveEdgeIndex + 1; ShiftIndex < ActiveEdgeCount; ++ShiftIndex)
				{
					ActiveEdges[ShiftIndex - 1] = ActiveEdges[ShiftIndex];
				}
				--ActiveEdgeCount;
			}
		}
		
		// Add active edges that start before the bottom of the scanline
		for (; EdgeIndex < EdgeCount; ++EdgeIndex)
		{
			glyph_edge Edge = Edges[EdgeIndex];
			
			// The edge is active if it is below the scanline
			if (Edge.Y1 <= ScanlineBottom)
			{
				f32 DXDY = (Edge.X2 - Edge.X1) / (Edge.Y2 - Edge.Y1);
				
				glyph_active_edge ActiveEdge;
				ActiveEdge.FX = Edge.X1 + DXDY * (ScanlineTop - Edge.Y1) - /* Offset */(f32)BBoxX1;
				ActiveEdge.FDeltaX = DXDY;
				ActiveEdge.FDeltaY = ((DXDY != 0.0f) ? (1.0f / DXDY) : 0.0f); // Safe divide, default 0
				ActiveEdge.Direction = (EdgesYSwap[EdgeIndex] ? 1.0f : -1.0f);
				ActiveEdge.SY = Edge.Y1;
				ActiveEdge.EY = Edge.Y2;
				
				if ((ScanlineIndex == 0) && (BBoxY1 != 0))
				{
					if (ActiveEdge.EY < ScanlineTop)
					{
						ActiveEdge.EY = ScanlineTop;
					}
				}
				
				Assert((ActiveEdgeCount + 1) <= EdgeCount);
				ActiveEdges[ActiveEdgeCount++] = ActiveEdge;
				
				// @Note Removing this Assert because it fires, fp error?
				// Assert(ActiveEdge.EY >= ScanlineTop);
			}
			else
			{
				// Other edges cannot be active
				break;
			}
		}
		
		//
		// stb_truetype rasterizer
		// https://nothings.org/gamedev/rasterize/
		// https://github.com/nothings/stb/blob/master/stb_truetype.h
		//
		
		f32 ScanLine1[FONT_CELL_WIDTH + (FONT_CELL_WIDTH + 1)] = {};
		f32 *ScanLine2 = ScanLine1 + FONT_CELL_WIDTH;
		
		for (smm ActiveEdgeIndex = ActiveEdgeCount - 1; ActiveEdgeIndex >= 0; --ActiveEdgeIndex)
		{
			glyph_active_edge *Edge = &ActiveEdges[ActiveEdgeIndex];
			
			//
			// Compute intersection points with top & bottom
			//
			
			Assert(Edge->EY >= ScanlineTop);
			
			if (Edge->FDeltaX == 0)
			{
				f32 X1 = Edge->FX;
				if (X1 < (f32)BBoxWidth)
				{
					if (X1 >= 0)
					{
						HandleClippedEdge(ScanLine1, (smm)X1 + 0, Edge, X1, ScanlineTop, X1, ScanlineBottom);
						HandleClippedEdge(ScanLine2, (smm)X1 + 1, Edge, X1, ScanlineTop, X1, ScanlineBottom);
					}
					else
					{
						HandleClippedEdge(ScanLine2, /* PADD */ 0, Edge, X1, ScanlineTop, X1, ScanlineBottom);
					}
				}
			}
			else
			{
				f32 X0 = Edge->FX;
				f32 DX = Edge->FDeltaX;
				f32 XB = X0 + DX;
				f32 DY = Edge->FDeltaY;
				
				Assert((Edge->SY <= ScanlineBottom) && 
					   (Edge->EY >= ScanlineTop));
				
				f32 XTop = ((Edge->SY > ScanlineTop) ? 
					(X0 + DX * (Edge->SY - ScanlineTop)) : X0);
				
				f32 XBottom = ((Edge->EY < ScanlineBottom) ? 
					(X0 + DX * (Edge->EY - ScanlineTop)) : XB);
				
				f32 SY0 = ((Edge->SY > ScanlineTop)    ? Edge->SY : ScanlineTop);
				f32 SY1 = ((Edge->EY < ScanlineBottom) ? Edge->EY : ScanlineBottom);
				
				#define GetTrapezoidArea(Height, TX0, TX1, BX0, BX1)\
					(  ( ( (TX1) - (TX0) ) + ( (BX1) - (BX0) ) ) * 0.5f * (Height)  )
				
				if ((XTop >= 0) && (XBottom >= 0) && (XTop < (f32)BBoxWidth) && (XBottom < (f32)BBoxWidth))
				{
					if ((smm)XTop == (smm)XBottom)
					{
						// Simple case, only spans one pixel
						smm X = (smm)XTop;
						f32 Height = (SY1 - SY0) * Edge->Direction;
						Assert((X >= 0) && (X < BBoxWidth));
						
						ScanLine1[X + 0] += GetTrapezoidArea(Height, XTop, ((f32)X + 1.0f), XBottom, ((f32)X + 1.0f));
						ScanLine2[X + 1] += Height; // Everything right of this pixel is filled
					}
					else
					{
						// Covers 2+ pixels
						if (XTop > XBottom)
						{
							// Flip scanline vertically. Signed area stays the same
							SY0		= ScanlineBottom - (SY0 - ScanlineTop);
							SY1		= ScanlineBottom - (SY1 - ScanlineTop);
							
							f32 Temp;
							Temp	= SY0;
							SY0		= SY1;
							SY1		= Temp;
							
							Temp	= XBottom;
							XBottom	= XTop;
							XTop	= Temp;
							
							DX		= -DX;
							DY		= -DY;
							
							Temp	= X0;
							X0		= XB;
							XB		= Temp;
						}
						
						Assert(DY >= 0.0f);
						Assert(DX >= 0.0f);
						
						smm X1 = (smm)XTop;
						smm X2 = (smm)XBottom;
						// Compute intersection with Y axis at (X1 + 1)
						f32 YCrossing = ScanlineTop + DY * ((f32)(X1 + 1) - X0);
						
						// Compute intersection with Y axis at X2
						f32 YFinal = ScanlineTop + DY * ((f32)X2 - X0);
						
						YCrossing = Min(YCrossing, ScanlineBottom);
						
						f32 Sign = Edge->Direction;
						
						// Area of the rectangle covered from SY0..YCrossing
						f32 Area = Sign * (YCrossing - SY0);
						
						// Area of the triangle (XTop,SY0), (X1+1,SY0), (X1+1,YCrossing)
						ScanLine1[X1] += (Area * ((f32)(X1 + 1) - XTop)) * 0.5f;
						
						// Check if final YCrossing is blown up
						if (YFinal > ScanlineBottom)
						{
							YFinal = ScanlineBottom;
							DY = (YFinal - YCrossing) / (f32)(X2 - (X1 + 1));
						}
						
						f32 Step = Sign * DY * 1.0f;
						
						for (smm X = X1 + 1; X < X2; ++X)
						{
							// Area of trapezoid is (1 * (Step / 2))
							ScanLine1[X] += Area + (Step * 0.5f);
							
							Area += Step;
						}
						
						Assert(Abs(Area) <= 1.01f); // Accumulated error from Area += Step unless we round Step down
						Assert(SY1 > (YFinal - 0.01f));
						
						// Area covered in the last pixel is the rectangle from all the pixels to the left,
						// plus the trapezoid filled by the line segment in this pixel all the way to the right edge
						f32 TrapezoidArea = GetTrapezoidArea((SY1 - YFinal), (f32)X2, (f32)(X2 + 1), XBottom, (f32)(X2 + 1));
						ScanLine1[X2] += Area + Sign * TrapezoidArea;
						
						// The rest of the line is filled based on the total height of the line segment in this pixel
						ScanLine2[X2 + 1] += Sign * (SY1 - SY0);
					}
				}
				else
				{
					// Uncommon scenario
					for (smm X = 0; X < BBoxWidth; ++X) {
						// Rename variables to clearly-defined pairs
						f32 Y0 = ScanlineTop;
						f32 X1 = (f32)(X + 0);
						f32 X2 = (f32)(X + 1);
						f32 X3 = XB;
						f32 Y3 = ScanlineBottom;
						
						f32 Y1 = ((f32)(X + 0) - X0) / DX + ScanlineTop;
						f32 Y2 = ((f32)(X + 1) - X0) / DX + ScanlineTop;
						
						if (X0 < X1 && X3 > X2)
						{
							// Three segments descending down-right
							HandleClippedEdge(ScanLine1, X, Edge, X0, Y0, X1, Y1);
							HandleClippedEdge(ScanLine1, X, Edge, X1, Y1, X2, Y2);
							HandleClippedEdge(ScanLine1, X, Edge, X2, Y2, X3, Y3);
						}
						else if (X3 < X1 && X0 > X2)
						{
							// Three segments descending down-left
							HandleClippedEdge(ScanLine1, X, Edge, X0, Y0, X2, Y2);
							HandleClippedEdge(ScanLine1, X, Edge, X2, Y2, X1, Y1);
							HandleClippedEdge(ScanLine1, X, Edge, X1, Y1, X3, Y3);
						}
						else if (X0 < X1 && X3 > X1)
						{
							// Two segments across X, down-right
							HandleClippedEdge(ScanLine1, X, Edge, X0, Y0, X1, Y1);
							HandleClippedEdge(ScanLine1, X, Edge, X1, Y1, X3, Y3);
						}
						else if (X3 < X1 && X0 > X1)
						{
							// Two segments across X, down-left
							HandleClippedEdge(ScanLine1, X, Edge, X0, Y0, X1, Y1);
							HandleClippedEdge(ScanLine1, X, Edge, X1, Y1, X3, Y3);
						}
						else if (X0 < X2 && X3 > X2)
						{
							// Two segments across X+1, down-right
							HandleClippedEdge(ScanLine1, X, Edge, X0, Y0, X2, Y2);
							HandleClippedEdge(ScanLine1, X, Edge, X2, Y2, X3, Y3);
						}
						else if (X3 < X2 && X0 > X2)
						{
							// Two segments across X+1, down-left
							HandleClippedEdge(ScanLine1, X, Edge, X0, Y0, X2, Y2);
							HandleClippedEdge(ScanLine1, X, Edge, X2, Y2, X3, Y3);
						}
						else
						{
							// One segment
							HandleClippedEdge(ScanLine1, X, Edge, X0, Y0, X3, Y3);
						}
					}
				}
			}
		}
		
		//
		// Write pixels to the texture buffer
		//
		
		f32 Sum = 0;
		for (smm X = 0; X < BBoxWidth; ++X)
		{
			Sum += ScanLine2[X];
			
			f32 K = ScanLine1[X] + Sum;
			K = Abs(K) * 255.0f + 0.5f;
			
			u32 M = (u32)K;
			M = Min(M, 255);
			
			u8 *ImageLine = OutputImageU8 + ScanlineIndex * FONT_CELL_WIDTH;
			
			ImageLine[X] = (u8)M;
		}
		
		// Advance all active edges
		for (smm Index = 0; Index < ActiveEdgeCount; ++Index)
		{
			ActiveEdges[Index].FX += ActiveEdges[Index].FDeltaX;
		}
		
		// Advance the scanline y
		ScanlineY += 1.0f;
	}
	
	// Normalize the FontImage from single channel monochromic R to RGBA
	u32 *OutputImageU32 = ArenaAlloc(TempArena, u32, FONT_CELL_WIDTH * FONT_CELL_HEIGHT);
	for (smm Index = 0; Index < (FONT_CELL_WIDTH * FONT_CELL_HEIGHT); ++Index)
	{
		u8 *OutputImageChannels = (u8 *)(OutputImageU32 + Index);
		u8 ColorValue = OutputImageU8[Index];
		OutputImageChannels[0] = ColorValue;
		OutputImageChannels[1] = ColorValue;
		OutputImageChannels[2] = ColorValue;
		OutputImageChannels[3] = ColorValue;
	}
	
	//
	// Find the advance width of the glyph
	//
	
	smm HMetricIndex = Min(GlyphIndex, Font->HMetricCount);
	// u16 AdvanceWidth; u16 LeftSideBearing;
	READ_Offset = Font->HmtxOffset + HMetricIndex * sizeof(u16) * 2;
	smm AdvanceWidth = Read16();
	
	//
	// Find a slot for the glyph in the atlas
	//
	
	smm LoadedGlyphIndex;
	if (Font->LoadedGlyphCount == Font->MaxLoadedGlyphCount)
	{
		// Find the most irrelevant glyph to replace
		// starting with an unreserved glyph
		LoadedGlyphIndex = Font->ReservedGlyphCount;
		smm ReplaceUsedFrame = Font->LoadedGlyphsUsedFrame[LoadedGlyphIndex];
		for (smm LoadedIndex = LoadedGlyphIndex; 
			LoadedIndex < Font->LoadedGlyphCount; 
			++LoadedIndex)
		{
			smm UsedFrame = Font->LoadedGlyphsUsedFrame[LoadedIndex];
			if (UsedFrame < ReplaceUsedFrame)
			{
				ReplaceUsedFrame = UsedFrame;
				LoadedGlyphIndex = LoadedIndex;
			}
		}
	}
	else
	{
		LoadedGlyphIndex = Font->LoadedGlyphCount++;
	}
	Font->LoadedGlyphsUsedFrame[LoadedGlyphIndex] = GLOBAL_MainState->Frame;
	Font->LoadedGlyphsCodepoint[LoadedGlyphIndex] = GlyphIndex;
	
	loaded_glyph *Glyph = &Font->LoadedGlyphsData[LoadedGlyphIndex];
	Glyph->Width	= (u8)BBoxWidth;
	Glyph->Height	= (u8)BBoxHeight;
	Glyph->OffsetX	= (s8)BBoxX1;
	Glyph->OffsetY	= (s8)-BBoxY2;
	Glyph->Advance	= (u8)FloorF32((f32)AdvanceWidth * Scale);
	
	Print("Width   %d", Glyph->Width);
	Print("Height  %d", Glyph->Height);
	Print("OffsetX %d", Glyph->OffsetX);
	Print("OffsetY %d", Glyph->OffsetY);
	Print("Advance %d", Glyph->Advance);
	
	//
	// Send the pixels to the GPU texture atlas
	//
	
	GLint TextureX = (GLint)((LoadedGlyphIndex &  Font->LoadedGlyphMask)  * FONT_CELL_WIDTH);
	GLint TextureY = (GLint)((LoadedGlyphIndex >> Font->LoadedGlyphShift) * FONT_CELL_HEIGHT);
	
	glTexSubImage2D(
		GL_TEXTURE_2D, 0, 
		TextureX, TextureY, FONT_CELL_WIDTH, FONT_CELL_HEIGHT, // (GLint)BBoxWidth, (GLint)BBoxHeight
		PLATFORM_OPENGL_UPLOAD_FORMAT, GL_UNSIGNED_BYTE, OutputImageU32
	);
	
	return LoadedGlyphIndex;
}

internal inline void ParseTTF()
{
	font *Font = &GLOBAL_MainState->Font;
	memory_arena *FontArena = &Font->Arena;
	
	// @Todo Dynamically choosing font size
	Font->FontSize = 64.0f;
	
	//
	// Atlas sizing
	//
	
	Font->MaxLoadedGlyphCount	= 128;
	
	smm GlyphSectionWidth		= TEXTURE_SIZE;
	smm GlyphCountX			= GlyphSectionWidth / FONT_CELL_WIDTH;
	// smm GlyphCountY			= Font->MaxLoadedGlyphCount / GlyphCountX;
	// smm GlyphSectionHeight		= GlyphCountY * FONT_CELL_HEIGHT;
	// @Todo Use GlyphSectionHeight to position other texture atlas for images
	
	Font->LoadedGlyphMask		= (u8)(GlyphCountX - 1);
	Font->LoadedGlyphShift		= (u8)BitScanForwardU32(GlyphCountX);
	
	Font->ReservedGlyphCount	= 11; // null, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
	Font->LoadedGlyphCount		= 0;
	
	Font->LoadedGlyphsCodepoint	= ArenaAlloc(FontArena, 		 u32, Font->MaxLoadedGlyphCount);
	Font->LoadedGlyphsUsedFrame	= ArenaAlloc(FontArena, 		 u16, Font->MaxLoadedGlyphCount);
	Font->LoadedGlyphsData		= ArenaAlloc(FontArena, loaded_glyph, Font->MaxLoadedGlyphCount);
	
	Print("Mask %d, Shift %d", Font->LoadedGlyphMask, Font->LoadedGlyphShift);
	
	//
	// Parse the TTF metadata
	//
	
	u8 *READ_Data		= Font->FileData;
	smm READ_Size		= Font->FileSize;
	smm READ_Offset	= 0; // Offset in the TTF file, Read8,16,32,64 functions use this offset
	
	Print("READ_Data %lld, READ_Size %d, READ_Offset %d", READ_Data, READ_Size, READ_Offset);
	
	// Disable unused variable warnings for this part of the code
	// Because we parse unused fields as well
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-variable"
	
	u32 DirVersion			= Read32();
	u16 DirNumTables		= Read16();
	u16 DirSearchRange		= Read16();
	u16 DirEntrySelector	= Read16();
	u16 DirRangeShift		= Read16();
	
	Assert((DirVersion == 0x00010000) || (DirVersion == 0x4F54544F));
	
	for (smm DirIndex = 0; DirIndex < DirNumTables; ++DirIndex)
	{
		//
		// Table metadata from table directory
		//
		
		char TableTag[4];
		TableTag[0] = Read8();
		TableTag[1] = Read8();
		TableTag[2] = Read8();
		TableTag[3] = Read8();
		
		u32 TableCheckSum	= Read32();
		u32 TableOffset		= Read32();
		u32 TableLength		= Read32();
		
		smm TableEndOffset = READ_Offset;
		
		//
		// Parse the specific type of table
		//
		
		READ_Offset = TableOffset;
		if (*(u32 *)TableTag == *(u32 *)"head")
		{
			u16 HEAD_MajorVersion		= Read16();
			u16 HEAD_MinorVersion		= Read16();
			u32 HEAD_FontRevision		= Read32();
			u32 HEAD_ChecksumAdjustment	= Read32();
			u32 HEAD_MagicNumber		= Read32();
			u16 HEAD_Flags				= Read16();
			u16 HEAD_UnitsPerEm			= Read16();
			u64 HEAD_Created			= Read64();
			u64 HEAD_Modified			= Read64();
			u16 HEAD_XMin				= Read16();
			u16 HEAD_YMin				= Read16();
			u16 HEAD_XMax				= Read16();
			u16 HEAD_YMax				= Read16();
			u16 HEAD_MacStyle			= Read16();
			u16 HEAD_LowestRecPPEM		= Read16();
			u16 HEAD_FontDirectionHint	= Read16();
			u16 HEAD_IndexToLocFormat	= Read16();
			u16 HEAD_GlyphDataFormat	= Read16();
			
			// Store the format of the 'loca' table, index format can be 16-bit or 32-bit
			Font->IndexToLocFormat = HEAD_IndexToLocFormat;
		}
		else if (*(u32 *)TableTag == *(u32 *)"maxp")
		{
			u32 MAXP_Version	= Read32();
			u16 MAXP_NumGlyphs	= Read16();
			
			Font->GlyphCount = MAXP_NumGlyphs;
		}
		else if (*(u32 *)TableTag == *(u32 *)"cmap")
		{
			u16 CMAP_Version	= Read16();
			u16 CMAP_NumTables	= Read16();
			
			for (smm TableIndex = 0; TableIndex < CMAP_NumTables; ++TableIndex)
			{
				u16 CMAP_RecordPlatformID		= Read16();
				u16 CMAP_RecordEncodingID		= Read16();
				u32 CMAP_RecordSubtableOffset	= Read32();
				
				smm RecordTablesOffset = READ_Offset;
				if (CMAP_RecordPlatformID == 3) // Platform ID Windows
				{
					READ_Offset = TableOffset + CMAP_RecordSubtableOffset;
					smm CMAP_Format = Read16();
					
					if (CMAP_RecordEncodingID == 1) // BMP Unicode Plane
					{
						Assert(CMAP_Format == 4);
						
						u16 CMAP_Length				= Read16();
						u16 CMAP_Language			= Read16(); // Macintosh platform only
						u16 CMAP_SegCountX2			= Read16();
						u16 CMAP_SearchRange		= Read16();
						u16 CMAP_EntrySelector		= Read16();
						u16 CMAP_RangeShift			= Read16();
						
						// @Todo Is Big Endian swapping here even worth it?
						// We can just do it in UTF8ToCodepoint without wasting memory
						
						smm SegCount				= CMAP_SegCountX2 / 2;
						Font->CMAP_SegmentCount		= SegCount;
						Font->CMAP_EndCodes			= ArenaAlloc(FontArena, u16, SegCount);
						Font->CMAP_StartCodes		= ArenaAlloc(FontArena, u16, SegCount);
						Font->CMAP_IDDeltas			= ArenaAlloc(FontArena, s16, SegCount);
						Font->CMAP_IDRangeOffsets	= ArenaAlloc(FontArena, u16, SegCount);
						
						for (smm Index = 0; Index < SegCount; ++Index)
						{
							Font->CMAP_EndCodes[Index]			= Read16();
						}
						u16 CMAP_ReservedPadding = Read16();
						for (smm Index = 0; Index < SegCount; ++Index)
						{
							Font->CMAP_StartCodes[Index]		= Read16();
						}
						for (smm Index = 0; Index < SegCount; ++Index)
						{
							Font->CMAP_IDDeltas[Index]			= Read16();
						}
						for (smm Index = 0; Index < SegCount; ++Index)
						{
							Font->CMAP_IDRangeOffsets[Index]	= Read16();
						}
					}
				}
				
				// Reset the offset after parsing the record
				// so that we have the next record metadata ready
				READ_Offset = RecordTablesOffset;
			}
		}
		else if (*(u32 *)TableTag == *(u32 *)"hhea")
		{
			u16 HHEA_MajorVersion			= Read16();
			u16 HHEA_MinorVersion			= Read16();
			s16 HHEA_Ascender				= Read16();
			s16 HHEA_Descender				= Read16();
			u16 HHEA_LineGap				= Read16();
			u16 HHEA_AdvanceWidthMax		= Read16();
			u16 HHEA_MinLeftSideBearing		= Read16();
			u16 HHEA_MinRightSideBearing	= Read16();
			u16 HHEA_XMaxExtent				= Read16();
			u16 HHEA_CaretSlopeRise			= Read16();
			u16 HHEA_CaretSlopeRun			= Read16();
			u16 HHEA_CaretOffset			= Read16();
			u16 HHEA_Reserved0				= Read16();
			u16 HHEA_Reserved1				= Read16();
			u16 HHEA_Reserved2				= Read16();
			u16 HHEA_Reserved3				= Read16();
			u16 HHEA_MetricDataFormat		= Read16();
			u16 HHEA_NumberOfHMetrics		= Read16();
			
			Font->HMetricCount = HHEA_NumberOfHMetrics;
			Print("HHEA_Ascender  %d", HHEA_Ascender);
			Print("HHEA_Descender %d", HHEA_Descender);
			Font->ScaleFactor = 1.0f / (f32)(HHEA_Ascender - HHEA_Descender);
		}
		else if (*(u32 *)TableTag == *(u32 *)"loca")
		{
			Font->LocaOffset = READ_Offset;
		}
		else if (*(u32 *)TableTag == *(u32 *)"hmtx")
		{
			Font->HmtxOffset = READ_Offset;
		}
		else if (*(u32 *)TableTag == *(u32 *)"glyf")
		{
			Font->GlyfOffset = READ_Offset;
		}
		
		// Reset the offset back to the next table record
		READ_Offset = TableEndOffset;
	}
	
	// Stops disabling Unused variable warnings
	#pragma clang diagnostic pop
}

internal umm UnicodeToGlyphID(umm UnicodeCodePoint)
{
	umm GlyphID = 0;
	if (UnicodeCodePoint <= 0xFFFF)
	{
		font *Font = &GLOBAL_MainState->Font;
		
		umm SearchRange		= RoundDownToPowerOfTwo((s32)Font->CMAP_SegmentCount);
		umm RangeShift		= Font->CMAP_SegmentCount - SearchRange;
		umm EntrySelector	= BitScanForwardU32((u32)SearchRange);
		
		smm EndIndex = 0;
		if (UnicodeCodePoint >= Font->CMAP_EndCodes[RangeShift])
		{
			EndIndex += RangeShift;
		}
		
		--EndIndex;
		while (EntrySelector != 0)
		{
			SearchRange >>= 1;
			umm End = Font->CMAP_EndCodes[EndIndex + SearchRange];
			if (UnicodeCodePoint > End)
			{
				EndIndex += SearchRange;
			}
			--EntrySelector;
		}
		++EndIndex;
		
		umm Start	= Font->CMAP_StartCodes[EndIndex];
		umm Last	= Font->CMAP_EndCodes[EndIndex];
		if ((UnicodeCodePoint >= Start) && 
			(UnicodeCodePoint <= Last))
		{
			umm Offset = Font->CMAP_IDRangeOffsets[EndIndex];
			if (Offset == 0)
			{
				GlyphID = UnicodeCodePoint + Font->CMAP_IDDeltas[EndIndex];
			}
			else
			{
				GlyphID = Font->CMAP_IDRangeOffsets[Offset + (UnicodeCodePoint - Start)];
			}
		}
	}
	
	return GlyphID;
}

internal inline umm UTF8ToCodepoint(u8 *Input, smm *OutIndex)
{
	Assert(Input != nullptr);
	Assert(OutIndex != nullptr);
	
	#define UTF8_ONE_BYTE_MASK      0b10000000 // 0xxxxxxx
	#define UTF8_ONE_BYTE_HEADER    0b00000000
	
	#define UTF8_TWO_BYTE_MASK      0b11100000 // 110xxxxx
	#define UTF8_TWO_BYTE_HEADER    0b11000000
	
	#define UTF8_THREE_BYTE_MASK    0b11110000 // 1110xxxx
	#define UTF8_THREE_BYTE_HEADER  0b11100000
	
	#define UTF8_FOUR_BYTE_MASK     0b11111000 // 11110xxx
	#define UTF8_FOUR_BYTE_HEADER   0b11110000
	
	#define UTF8_EXTRA_BYTE_MASK    0b11000000 // 10xxxxxx
	#define UTF8_EXTRA_BYTE_HEADER	0b10000000
	
	umm CharacterUTF32 = 0;
	smm Index = *OutIndex;
	if ((Input[Index] & UTF8_ONE_BYTE_MASK) == UTF8_ONE_BYTE_HEADER)
	{
		CharacterUTF32 = Input[Index];
		
		*OutIndex += 1;
	}
	else if ((Input[Index] & UTF8_TWO_BYTE_MASK) == UTF8_TWO_BYTE_HEADER)
	{
		CharacterUTF32 = (((Input[Index + 0] & ~UTF8_TWO_BYTE_MASK)   << (6 * 1)) | 
						  ((Input[Index + 1] & ~UTF8_EXTRA_BYTE_MASK) << (6 * 0)));
		
		*OutIndex += 2;
	}
	else if ((Input[Index] & UTF8_THREE_BYTE_MASK) == UTF8_THREE_BYTE_HEADER)
	{
		CharacterUTF32 = (((Input[Index + 0] & ~UTF8_THREE_BYTE_MASK) << (6 * 2)) | 
						  ((Input[Index + 1] & ~UTF8_EXTRA_BYTE_MASK) << (6 * 1)) | 
						  ((Input[Index + 2] & ~UTF8_EXTRA_BYTE_MASK) << (6 * 0)));
		
		*OutIndex += 3;
	}
	else if ((Input[Index] & UTF8_FOUR_BYTE_MASK) == UTF8_FOUR_BYTE_HEADER)
	{
		CharacterUTF32 = (((Input[Index + 0] & ~UTF8_FOUR_BYTE_MASK)  << (6 * 3)) | 
						  ((Input[Index + 1] & ~UTF8_EXTRA_BYTE_MASK) << (6 * 2)) | 
						  ((Input[Index + 2] & ~UTF8_EXTRA_BYTE_MASK) << (6 * 1)) | 
						  ((Input[Index + 3] & ~UTF8_EXTRA_BYTE_MASK) << (6 * 0)));
		
		*OutIndex += 4;
	}
	else
	{
		Assert(false);
	}
	
	return CharacterUTF32;
}
