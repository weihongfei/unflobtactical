#include "modelbuilder.h"
#include "../engine/vertex.h"
#include "../grinliz/glgeometry.h"


using namespace grinliz;

void ModelBuilder::SetTexture( const char* textureName )
{
	GLASSERT( strlen( textureName ) < 16 );
	current = 0;

	for( int i=0; i<nGroup; ++i ) {
		if ( strcmp( textureName, group[i].textureName ) == 0 ) {
			current = &group[i];
			break;
		}
	}
	if ( !current ) {
		GLASSERT( nGroup < EL_MAX_MODEL_GROUPS );
		strcpy( group[nGroup].textureName, textureName );
		current = &group[nGroup];
		++nGroup;
	}
}


void ModelBuilder::AddTri( const Vertex& v0, const Vertex& v1, const Vertex& v2 )
{
	GLASSERT( current );
	GLASSERT( current->nIndex < EL_MAX_INDEX_IN_GROUP-3 );

	const Vertex v[3] = { v0, v1, v2 };
	VertexX vX[3];

	// Transform the vertex into the current coordinate space.
	for( int i=0; i<3; ++i ) {
		Vertex vert = v[i];
		MultMatrix4( matrix, v[i].pos, &vert.pos );
		//Trouble if there is ever rotation, because the normals shouldn't be translated.
		GLASSERT( matrix.IsTranslationOnly() );
		vert.normal.Normalize();

		vX[i].From( vert );
	}

	const float CLOSE = 0.8f;

	for( int i=0; i<3; ++i ) 
	{
//		int start = grinliz::Max( (int)0, current->nVertex - SCAN_BACK );
		int start = 0;
		bool added = false;

		for( int j=start; j<current->nVertex; ++j ) 
		{
			const VertexX& vc = current->vertex[j];

			if (    vc.pos == vX[i].pos
				 && vc.tex == vX[i].tex )
			{
				// We might have a match. Look at the normals. Are they
				// similar enough?
				Vector3F normal0 = { FixedToFloat( vc.normal.x ), FixedToFloat( vc.normal.y ), FixedToFloat( vc.normal.z ) };
				Vector3F normal1 = {	FixedToFloat( vX[i].normal.x ), 
										FixedToFloat( vX[i].normal.y ), 
										FixedToFloat( vX[i].normal.z ) };

				float dot = DotProduct( normal0, normal1 );

				if ( dot > CLOSE ) {
					added = true;
					current->index[ current->nIndex ] = j;
					current->normalSum[j] = current->normalSum[j] + normal1;
					current->nIndex++;
					break;
				}
			}
		}
		if ( !added ) {
			GLASSERT( current->nVertex < EL_MAX_VERTEX_IN_GROUP );
			current->normalSum[ current->nVertex ].Set( FixedToFloat( vX[i].normal.x ), 
														FixedToFloat( vX[i].normal.y ), 
														FixedToFloat( vX[i].normal.z ) );
			current->index[ current->nIndex++ ] = current->nVertex;
			current->vertex[ current->nVertex++ ] = vX[i];
		}
	}
	// Always add 3 indices.
	GLASSERT( current->nIndex % 3 == 0 );
}


void ModelBuilder::Flush()
{
	for( int i=0; i<nGroup; ++i ) {
		for( int j=0; j<group[i].nVertex; ++j ) {
			group[i].normalSum[j].Normalize();
			group[i].vertex[j].normal.Set(	FloatToFixed( group[i].normalSum[j].x ),
											FloatToFixed( group[i].normalSum[j].y ),
											FloatToFixed( group[i].normalSum[j].z ) );

		}
	}
}
