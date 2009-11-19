/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFOATTACK_MAP_INCLUDED
#define UFOATTACK_MAP_INCLUDED

#include <stdio.h>
#include <string>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glbitarray.h"
#include "../grinliz/glmemorypool.h"
#include "../micropather/micropather.h"
#include "vertex.h"
#include "surface.h"
#include "enginelimits.h"
#include "serialize.h"

class Model;
class ModelResource;
class SpaceTree;
class RenderQueue;
class Texture;
class ItemDef;
class Storage;
class Game;


class Map : public micropather::Graph
{
public:
	enum {
		SIZE = 64,					// maximum size. FIXME: duplicated in gamelimits.h
		LOG2_SIZE = 6,

		MAX_ITEM_DEF = 256,
		MAX_TRAVEL = 16,
	};

	struct MapItemDef 
	{
		enum { MAX_CX = 6, MAX_CY = 6 };

		void Init() {	name[0] = 0;
						
						cx = 1; 
						cy = 1; 
						hp = 0; 
						materialFlags = 0;

						modelResource			= 0;
						modelResourceOpen		= 0;
						modelResourceDestroyed	= 0;

						memset( pather, 0, MAX_CX*MAX_CY );
						memset( visibility, 0, MAX_CX*MAX_CY );
					}

		U16		cx, cy;
		U16		hp;					// 0xffff infinite, 0 destroyed
		U8		transparency;		// 0 opaque - 255 transparent
		U8		_pad;
		U16		materialFlags;

		const ModelResource* modelResource;
		const ModelResource* modelResourceOpen;
		const ModelResource* modelResourceDestroyed;

		char	name[EL_FILE_STRING_LEN];
		U8		pather[MAX_CX][MAX_CY];
		U8		visibility[MAX_CX][MAX_CY];

		// return true if the object can take damage
		bool CanDamage() const { return hp != 0xffff; }
	};

	struct MapItem
	{
		U8		x, y, rot;
		U8		itemDefIndex;	// 0: not in use, >0 is the index
		U16		hp;
		U16		flags;

		grinliz::Rectangle2<U8> mapBounds8;
		
		Model*	 model;
		Storage* storage;
		
		MapItem* next;			// the 'next' after a query
		MapItem* nextQuad;		// next pointer in the quadTree


		void MapBounds( grinliz::Rectangle2I* r ) const 
		{
			r->Set( mapBounds8.min.x, mapBounds8.min.y, mapBounds8.max.x, mapBounds8.max.y );
		}

		bool InUse() const			{ return itemDefIndex > 0; }

		// returns true if destroyed
		bool DoDamage( int _hp )		
		{	
			if ( _hp >= hp ) {
				hp = 0;
				return true;
			}
			hp -= _hp;
			return false;						
		}
		bool Destroyed() { return hp == 0; }
	};

	/* FIXME: The map lives between the game and the engine. It should probably be moved
	   to the Game layer. Until then, it takes an engine primitive (SpaceTree) and the game
	   basic class (Game) when it loads. Very strange.
	*/
	Map( SpaceTree* tree );
	virtual ~Map();

	// The size of the map in use, which is <=SIZE
	int Height() { return height; }
	int Width()  { return width; }

	void SetSize( int w, int h )					{ width = w; height = h; }

	// The background texture of the map. The map is just one big tetxure.
	void SetTexture( const Texture* texture )		{ this->texture = texture; }

	// The light map is a 64x64 texture of the lighting at each point. Without
	// a light map, full white (daytime) is used. GenerateLightMap creates the
	// resulting map by combining light with FogOfWar and lights in the world.
	// GenerateLightMap also creates the translucency map, which is used to 
	// compute the visibility rays.
	void SetLightMap( const Surface* surface );

	const grinliz::BitArray<Map::SIZE, Map::SIZE, 1>&	GetFogOfWar()		{ return fogOfWar; }
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1>*			LockFogOfWar();
	void												ReleaseFogOfWar();

	const Surface* GetLightMap()	{ GenerateLightMap(); return &finalMap; }

	// Rendering.
	void BindTextureUnits();
	void UnBindTextureUnits();
	void Draw();
	void DrawPath();
	void DrawOverlay();

	// Explosions impacts and such.
	void DoDamage( int damageBase, Model* m, int shellFlags );

	// Sets objects to block the path (usually other sprites) that the map doesn't know about.
	void ClearPathBlocks();
	void SetPathBlock( int x, int y );

	void SetStorage( int x, int y, Storage* storage );
	Storage* RemoveStorage( int x, int y );

	MapItemDef* InitItemDef( int i );
	const char* GetItemDefName( int i );

#ifdef MAPMAKER
	Model* CreatePreview( int x, int z, int itemDefIndex, int rotation );
#endif
	// hp = -1 default
	//       0 destroyed
	//		1+ hp remaining
	// Storage is owned by the map after this call.
	bool AddItem( int x, int z, int rotation, int itemDefIndex, int hp, int flags, Storage* storage );
	void DeleteAt( int x, int z );
	void MapBoundsOfModel( const Model* m, grinliz::Rectangle2I* mapBounds );

	void ResetPath();	// normally called automatically

	static sqlite3* CreateMap( sqlite3* resourceDB );
	void SyncToDB( sqlite3* db, const char* table );
	void Clear();

	void DumpTile( int x, int z );

	// Solves a path on the map. Returns total cost. 
	// returns MicroPather::SOLVED, NO_SOLUTION, START_END_SAME, or OUT_OF_MEMORY
	int SolvePath(	const grinliz::Vector2<S16>& start,
					const grinliz::Vector2<S16>& end,
					float* cost,
					std::vector< void* >* path );
	
	void ShowNearPath(	const grinliz::Vector2<S16>& start,
						float cost0, float cost1, float cost2 );
	void ClearNearPath();	

	// micropather:
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void  AdjacentCost( void* state, std::vector< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );

	// visibility (but similar to AdjacentCost conceptually)
	bool CanSee( const grinliz::Vector2I& p, const grinliz::Vector2I& delta );

private:
	struct IMat
	{
		int a, b, c, d, x, z;

		void Init( int w, int h, int r );
		void Mult( const grinliz::Vector2I& in, grinliz::Vector2I* out );
	};

	enum ConnectionType {
		PATH_TYPE,
		VISIBILITY_TYPE
	};

	int InvertPathMask( U32 m ) {
		U32 m0 = (m<<2) | (m>>2);
		return m0 & 0xf;
	}

	class QuadTree
	{
	public:
		enum {
			QUAD_DEPTH = 5,
			NUM_QUAD_NODES = 1+4+16+64+256,
		};

		QuadTree();
		void Clear();

		void Add( MapItem* );
		void Unlink( MapItem* );

		MapItem* FindItems( const grinliz::Rectangle2I& bounds );
		MapItem* FindItems( int x, int y ) { grinliz::Rectangle2I b; b.Set( x, y, x, y ); return FindItems( b ); }
		MapItem* FindItem( const Model* model );

		void UnlinkItem( MapItem* item );

	private:
		int WorldToNode( int x, int depth )					
		{ 
			GLASSERT( depth >=0 && depth < QUAD_DEPTH );
			GLASSERT( x>=0 && x < Map::SIZE );
			return x >> (LOG2_SIZE-depth); 
		}
		int NodeToWorld( int x0, int depth )
		{
			GLASSERT( x0>=0 && x0 < (1<<depth) );
			return x0 << (LOG2_SIZE-depth);			
		}
		int NodeOffset( int x0, int y0, int depth )	
		{	
			GLASSERT( x0>=0 && x0 < (1<<depth) );
			GLASSERT( y0>=0 && y0 < (1<<depth) );
			return y0 * (1<<depth) + x0; 
		}

		int CalcNode( const grinliz::Rectangle2<U8>& bounds, int* depth );

		int			depthUse[QUAD_DEPTH];
		int			depthBase[QUAD_DEPTH+1];
		MapItem*	tree[NUM_QUAD_NODES];
		const Model* filterModel;
	};

	int GetPathMask( ConnectionType c, int x, int z );
	bool Connected( ConnectionType c, int x, int y, int dir );

	void StateToVec( const void* state, grinliz::Vector2<S16>* vec ) { *vec = *((grinliz::Vector2<S16>*)&state); }
	void* VecToState( const grinliz::Vector2<S16>& vec )			 { return (void*)(*(int*)&vec); }

	void CalcModelPos(	int x, int y, int r, const MapItemDef& itemDef, 
						grinliz::Rectangle2I* mapBounds,
						grinliz::Vector2F* origin );

	void ClearVisPathMap( grinliz::Rectangle2I& bounds );
	void CalcVisPathMap( grinliz::Rectangle2I& bounds );

	void InsertRow( int x, int y, int r, int def, int hp, int flags, const Storage* storage );
	void DeleteRow( int x, int y, int r, int def );


	int width, height;
	grinliz::Rectangle3F bounds;
	const Texture* texture;
	SpaceTree* tree;

	sqlite3* mapDB;
	std::string dbTableName;

	Vertex vertex[4];
	grinliz::Vector2F texture1[4];

	void GenerateLightMap();
	Texture finalMapTex;	
	Surface finalMap;		// final light map, RGB16
	Surface baseMap;		// base map, RGB16
	grinliz::Rectangle2I invalidLightMap;	// [x,x0)
	grinliz::BitArray<Map::SIZE, Map::SIZE, 1> fogOfWar;

	U32 pathQueryID;
	U32 visibilityQueryID;

	micropather::MicroPather* microPather;

	grinliz::BitArray<SIZE, SIZE, 1>	pathBlock;	// spaces the pather can't use (units are there)	

	std::vector<void*>					mapPath;
	std::vector< micropather::StateCost > stateCostArr;
	grinliz::BitArray< SIZE, SIZE, 3 >	walkingMap;

	void PushWalkingVertex( int x, int z, float tx, float ty ) 
	{
		GLASSERT( nWalkingVertex < 6*MAX_TRAVEL*MAX_TRAVEL );
		walkingVertex[ nWalkingVertex ].normal.Set( 0.0f, 1.0f, 0.0f );
		walkingVertex[ nWalkingVertex ].pos.Set( (float)x, 0.0f,(float)(z) );
		walkingVertex[ nWalkingVertex ].tex.Set( tx, ty );
		++nWalkingVertex;
	}

	Vertex								walkingVertex[6*MAX_TRAVEL*MAX_TRAVEL];
	int									nWalkingVertex;

	U8									visMap[SIZE*SIZE];
	U8									pathMap[SIZE*SIZE];

	grinliz::MemoryPool					itemPool;
	QuadTree							quadTree;
	MapItemDef							itemDefArr[MAX_ITEM_DEF];
};

#endif // UFOATTACK_MAP_INCLUDED
