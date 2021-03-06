/*
 Copyright (c) 2010 Lee Thomason

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#include "gamui.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

using namespace gamui;
using namespace std;

static const float PI = 3.1415926535897932384626433832795f;

void Matrix::SetXRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	x[5] = cosTheta;
	x[6] = sinTheta;

	x[9] = -sinTheta;
	x[10] = cosTheta;
}


void Matrix::SetYRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = 0.0f;
	x[2] = -sinTheta;
	
	// COLUMN 2
	x[4] = 0.0f;
	x[5] = 1.0f;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = sinTheta;
	x[9] = 0;
	x[10] = cosTheta;
}

void Matrix::SetZRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = sinTheta;
	x[2] = 0.0f;
	
	// COLUMN 2
	x[4] = -sinTheta;
	x[5] = cosTheta;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = 0.0f;
	x[9] = 0.0f;
	x[10] = 1.0f;
}



UIItem::UIItem( int p_level ) 
	: m_x( 0 ),
	  m_y( 0 ),
	  m_level( p_level ),
	  m_visible( true ),
	  m_rotationX( 0 ),
	  m_rotationY( 0 ),
	  m_rotationZ( 0 ),
	  m_gamui( 0 ),
	  m_enabled( true )
{}


UIItem::~UIItem()
{
	if ( m_gamui )
		m_gamui->Remove( this );
}


Gamui::Vertex* UIItem::PushQuad( CDynArray< uint16_t > *indexBuf, CDynArray< Gamui::Vertex > *vertexBuf )
{
	int base = vertexBuf->Size();
	uint16_t *index = indexBuf->PushArr( 6 );

	index[0] = base+0;
	index[1] = base+1;
	index[2] = base+2;
	index[3] = base+0;
	index[4] = base+2;
	index[5] = base+3;

	return vertexBuf->PushArr( 4 );
}


void UIItem::ApplyRotation( int nVertex, Gamui::Vertex* vertex )
{
	if ( m_rotationX != 0.0f || m_rotationY != 0.0f || m_rotationZ != 0.0f ) {
		Matrix m, mx, my, mz, t0, t1, temp;

		t0.SetTranslation( -(X()+Width()*0.5f), -(Y()+Height()*0.5f), 0 );
		t1.SetTranslation( (X()+Width()*0.5f), (Y()+Height()*0.5f), 0 );
		mx.SetXRotation( m_rotationX );
		my.SetYRotation( m_rotationY );
		mz.SetZRotation( m_rotationZ );

		m = t1 * mz * my * mx * t0;

		for( int i=0; i<nVertex; ++i ) {
			float in[3] = { vertex[i].x, vertex[i].y, 0 };
			MultMatrix( m, in, 2, &vertex[i].x );
		}
	}
}


TextLabel::TextLabel() : UIItem( Gamui::LEVEL_TEXT ),
	m_width( -1 ),
	m_height( -1 )
{
	m_str = m_buf;
	m_str[0] = 0;
	m_allocated = ALLOCATED;
}


TextLabel::~TextLabel()
{
	if ( m_gamui ) 
		m_gamui->Remove( this );
	if ( m_str != m_buf )
		delete [] m_str;
}


void TextLabel::Init( Gamui* gamui )
{
	m_gamui = gamui;
	m_gamui->Add( this );
}


void TextLabel::ClearText()
{
	if ( m_str[0] ) {
		m_str[0] = 0;
		m_width = m_height = -1;
		Modify();
	}
}


const RenderAtom* TextLabel::GetRenderAtom() const
{
	GAMUIASSERT( m_gamui );
	GAMUIASSERT( m_gamui->GetTextAtom() );

	return Enabled() ? m_gamui->GetTextAtom() : m_gamui->GetDisabledTextAtom();
}


const char* TextLabel::GetText() const
{
	return m_str;
}


void TextLabel::SetText( const char* t )
{
	if ( t ) 
		SetText( t, t+strlen(t) );
	else
		SetText( "" );
}


void TextLabel::SetText( const char* start, const char* end )
{
	if (    memcmp( start, m_str, end-start ) == 0	// contain the same thing
		 && m_str[end-start] == 0 )					// have the same length
	{
		// They are the same.
	}
	else {
		m_width = m_height = -1;
		int len = end - start;
		int allocatedNeeded = len+1;

		if ( m_allocated < allocatedNeeded ) {
			m_allocated = (allocatedNeeded+ALLOCATED)*3/2;
			if ( m_str != m_buf )
				delete m_str;
			m_str = new char[m_allocated];
		}
		memcpy( m_str, start, len );
		m_str[len] = 0;
		Modify();
	}
}


bool TextLabel::DoLayout() 
{
	return m_str[0] != 0;
}


void TextLabel::CalcSize( float* width, float* height ) const
{
	*width = 0;
	*height = 0;

	if ( !m_gamui )
		return;
	IGamuiText* iText = m_gamui->GetTextInterface();
	if ( !iText )
		return;

	const char* p = m_str;

	IGamuiText::GlyphMetrics metrics;
	float x = 0;
	float h = m_gamui->GetTextHeight();

	while ( p && *p ) {
		iText->GamuiGlyph( *p, p>m_str ? *(p-1):0, h, &metrics );
		++p;
		x += metrics.advance;
	}
	*width = x;
	*height = m_gamui->GetTextHeight();
	GAMUIASSERT( *height > 0 );
}


void TextLabel::Queue( CDynArray< uint16_t > *indexBuf, CDynArray< Gamui::Vertex > *vertexBuf )
{
	if ( !m_gamui )
		return;
	IGamuiText* iText = m_gamui->GetTextInterface();

	const char* p = m_str;
	float x = X();
	float y = Y(); //floorf( Y()+0.5f );	// snapping seems to hurt quality. Text is so tricky.

	IGamuiText::GlyphMetrics metrics;
	float height = m_gamui->GetTextHeight();

	while ( p && *p ) {
		iText->GamuiGlyph( *p, p>m_str ? *(p-1):0, height, &metrics );

		Gamui::Vertex* vertex = PushQuad( indexBuf, vertexBuf );

		//x = floorf( x + 0.5f );
		float x0 = x+metrics.x;
		float x1 = x+metrics.x+metrics.w;
		float y0 = y+metrics.y;
		float y1 = y+metrics.y+metrics.h;

		vertex[0].Set( x0, y0,				
					   metrics.tx0, metrics.ty0 );
		vertex[1].Set( x0, y1, 
					   metrics.tx0, metrics.ty1 );
		vertex[2].Set( x1, y1, 
					   metrics.tx1, metrics.ty1 );
		vertex[3].Set( x1, y0,
			           metrics.tx1, metrics.ty0 );

		++p;
		x += metrics.advance;
	}
}


float TextLabel::Width() const
{
	if ( !m_gamui )
		return 0;

	if ( m_width < 0 ) {
		CalcSize( &m_width, &m_height );
	}
	return m_width;
}


float TextLabel::Height() const
{
	if ( !m_gamui )
		return 0;

	if ( m_height < 0 ) {
		CalcSize( &m_width, &m_height );
	}
	return m_height;
}


TextBox::TextBox() : UIItem( Gamui::LEVEL_TEXT ), m_needsLayout( true ), m_width( 0 ), m_height( 0 ), m_textLabelArr( 0 ), m_lines( 0 )
{}


TextBox::~TextBox() 
{
	delete [] m_textLabelArr;
}


void TextBox::Init( Gamui* g ) 
{
	m_gamui = g;
	m_gamui->Add( this );
	m_storage.Init( g );
	m_storage.SetVisible( false );
	m_needsLayout = true;
}


const RenderAtom* TextBox::GetRenderAtom() const
{
	return 0;
}


bool TextBox::DoLayout()
{
	if ( m_needsLayout ) {
		float h = m_storage.Height();
		int lines =(int)(Height()/h);

		if ( lines != m_lines ) {
			m_lines = lines;
			delete [] m_textLabelArr;
			m_textLabelArr = new TextLabel[m_lines];
			for( int i=0; i<m_lines; ++i ) {
				m_textLabelArr[i].Init( m_gamui );
			}
		}
		for( int i=0; i<m_lines; ++i ) {
			m_textLabelArr[i].SetVisible( Visible() );
			m_textLabelArr[i].SetEnabled( Enabled() );
		}
		m_gamui->LayoutTextBlock( m_storage.GetText(), m_textLabelArr, m_lines, X(), Y(), Width() );

		m_needsLayout = false;
	}
	return false;	// children render, not the textbox
}


void TextBox::Queue( CDynArray< uint16_t > *indexBuf, CDynArray< Gamui::Vertex > *vertexBuf )
{
	// does nothing - children draw
}


Image::Image() : UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( DEFAULT_SIZE ),
	  m_height( DEFAULT_SIZE ),
	  m_slice( false )
{
}


Image::Image( Gamui* gamui, const RenderAtom& atom, bool foreground ): UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( DEFAULT_SIZE ),
	  m_height( DEFAULT_SIZE ),
	  m_slice( false )
{
	Init( gamui, atom, foreground );
}

Image::~Image()
{
}


void Image::Init( Gamui* gamui, const RenderAtom& atom, bool foreground )
{
	m_atom = atom;
	m_width  = DEFAULT_SIZE;
	m_height = DEFAULT_SIZE;

	m_gamui = gamui;
	gamui->Add( this );
	this->SetForeground( foreground );
}


void Image::SetAtom( const RenderAtom& atom )
{
	if ( !atom.Equal( m_atom ) ) {
		m_atom = atom;
		Modify();
	}
}


void Image::SetSlice( bool enable )
{
	if ( enable != m_slice ) { // || w != m_sliceWidth || h != m_sliceHeight ) {
		m_slice = enable;
		//m_sliceWidth = w;
		//m_sliceHeight = h;
		Modify();
	}
}


void Image::SetForeground( bool foreground )
{
	this->SetLevel( foreground ? Gamui::LEVEL_FOREGROUND : Gamui::LEVEL_BACKGROUND );
}


bool Image::DoLayout()
{
	return true;
}


TiledImageBase::TiledImageBase() : UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 )
{
}


TiledImageBase::TiledImageBase( Gamui* gamui ): UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 )
{
	Init( gamui );
}


TiledImageBase::~TiledImageBase()
{
}


void TiledImageBase::Init( Gamui* gamui )
{
	m_width = DEFAULT_SIZE;
	m_height = DEFAULT_SIZE;

	m_gamui = gamui;
	gamui->Add( this );
}


void TiledImageBase::SetTile( int x, int y, const RenderAtom& atom )
{
	GAMUIASSERT( x<CX() );
	GAMUIASSERT( y<CY() );
	if ( x < 0 || x >= CX() || y < 0 || y >= CY() )
		return;

	int index = 0;

	if ( atom.textureHandle == 0 ) {
		// Can always add a null atom.
		index = 0;
	}
	else if ( m_atom[1].textureHandle == 0 ) {
		// First thing added.
		index = 1;
		m_atom[1] = atom;
	}
	else {
		GAMUIASSERT( atom.renderState == m_atom[1].renderState );
		GAMUIASSERT( atom.textureHandle == m_atom[1].textureHandle );
		for ( index=1; index<MAX_ATOMS && m_atom[index].textureHandle; ++index ) {
			if (    m_atom[index].tx0 == atom.tx0
				 && m_atom[index].ty0 == atom.ty0
				 && m_atom[index].tx1 == atom.tx1
				 && m_atom[index].ty1 == atom.ty1 )
			{
				break;
			}
		}
		m_atom[index] = atom;
	}
	*(Mem()+y*CX()+x) = index;
	Modify();
}


void TiledImageBase::SetForeground( bool foreground )
{
	this->SetLevel( foreground ? Gamui::LEVEL_FOREGROUND : Gamui::LEVEL_BACKGROUND );
}


const RenderAtom* TiledImageBase::GetRenderAtom() const
{
	return &m_atom[1];
}


void TiledImageBase::Clear()														
{ 
	memset( Mem(), 0, CX()*CY() ); 
	memset( m_atom, 0, sizeof(RenderAtom)*MAX_ATOMS );
	Modify();
}

bool TiledImageBase::DoLayout()
{
	return true;
}


void TiledImageBase::Queue( CDynArray< uint16_t > *indexBuf, CDynArray< Gamui::Vertex > *vertexBuf )
{
	int startVertex = vertexBuf->Size();

	int cx = CX();
	int cy = CY();
	float x = X();
	float y = Y();
	float dx = Width() / (float)cx;
	float dy = Height() / (float)cy;
	int8_t* mem = Mem();
	int count = 0;

	for( int j=0; j<cy; ++j ) {
		for( int i=0; i<cx; ++i ) {
			if (*mem >= 0 && *mem < MAX_ATOMS && m_atom[*mem].textureHandle ) {
				Gamui::Vertex* vertex = PushQuad( indexBuf, vertexBuf );

				vertex[0].Set( x,	y,		m_atom[*mem].tx0, m_atom[*mem].ty1 );
				vertex[1].Set( x,	y+dy,	m_atom[*mem].tx0, m_atom[*mem].ty0 );
				vertex[2].Set( x+dx, y+dy,	m_atom[*mem].tx1, m_atom[*mem].ty0 );
				vertex[3].Set( x+dx, y,		m_atom[*mem].tx1, m_atom[*mem].ty1 );

				++count;
			}
			x += dx;
			++mem;
		}
		x = X();
		y += dy;
	}
	ApplyRotation( count*4, vertexBuf->Mem() + startVertex );
}


const RenderAtom* Image::GetRenderAtom() const
{
	return &m_atom;
}


void Image::Queue( CDynArray< uint16_t > *indexBuf, CDynArray< Gamui::Vertex > *vertexBuf )
{
	if ( m_atom.textureHandle == 0 ) {
		return;
	}

	// Dislike magic numbers, but also dislike having to track atom sizes.
	float sliceSize = 0.75f * Min( m_width, m_height );

	if (    !m_slice
		 || ( m_width <= sliceSize && m_height <= sliceSize ) )
	{
		Gamui::Vertex* vertex = PushQuad( indexBuf, vertexBuf );

		float x0 = X();
		float y0 = Y();
		float x1 = X() + m_width;
		float y1 = Y() + m_height;

		vertex[0].Set( x0, y0, m_atom.tx0, m_atom.ty1 );
		vertex[1].Set( x0, y1, m_atom.tx0, m_atom.ty0 );
		vertex[2].Set( x1, y1, m_atom.tx1, m_atom.ty0 );
		vertex[3].Set( x1, y0, m_atom.tx1, m_atom.ty1 );
		ApplyRotation( 4, vertex );
	}
	else {
		float x[4] = { X(), X()+(sliceSize*0.5f), X()+(m_width-sliceSize*0.5f), X()+m_width };
		if ( x[2] < x[1] ) {
			x[2] = x[1] = X() + (sliceSize*0.5f);
		}
		float y[4] = { Y(), Y()+(sliceSize*0.5f), Y()+(m_height-sliceSize*0.5f), Y()+m_height };
		if ( y[2] < y[1] ) {
			y[2] = y[1] = Y() + (sliceSize*0.5f);
		}

		float tx[4] = { m_atom.tx0, Mean( m_atom.tx0, m_atom.tx1 ), Mean( m_atom.tx0, m_atom.tx1 ), m_atom.tx1 };
		float ty[4] = { m_atom.ty1, Mean( m_atom.ty0, m_atom.ty1 ), Mean( m_atom.ty0, m_atom.ty1 ), m_atom.ty0 };

		int base = vertexBuf->Size();
		Gamui::Vertex* vertex = vertexBuf->PushArr( 16 );

		for( int j=0; j<4; ++j ) {
			for( int i=0; i<4; ++i ) {
				vertex[j*4+i].Set( x[i], y[j], tx[i], ty[j] );
			}
		}
		ApplyRotation( 16, vertex );

		uint16_t* index = indexBuf->PushArr( 6*3*3 );
		int count=0;

		for( int j=0; j<3; ++j ) {
			for( int i=0; i<3; ++i ) {
				index[count++] = base + j*4+i;
				index[count++] = base + (j+1)*4+i;
				index[count++] = base + (j+1)*4+(i+1);

				index[count++] = base + j*4+i;
				index[count++] = base + (j+1)*4+(i+1);
				index[count++] = base + j*4+(i+1);
			}
		}
	}
}




Button::Button() : UIItem( Gamui::LEVEL_FOREGROUND ),
	m_up( true ),
	m_usingText1( false )
{
}


void Button::Init(	Gamui* gamui,
					const RenderAtom& atomUpEnabled,
					const RenderAtom& atomUpDisabled,
					const RenderAtom& atomDownEnabled,
					const RenderAtom& atomDownDisabled,
					const RenderAtom& decoEnabled, 
					const RenderAtom& decoDisabled )
{
	m_gamui = gamui;

	m_atoms[UP] = atomUpEnabled;
	m_atoms[UP_D] = atomUpDisabled;
	m_atoms[DOWN] = atomDownEnabled;
	m_atoms[DOWN_D] = atomDownDisabled;
	m_atoms[DECO] = decoEnabled;
	m_atoms[DECO_D] = decoDisabled;

	m_textLayout = CENTER;
	m_decoLayout = CENTER;
	m_textDX = 0;
	m_textDY = 0;
	m_decoDX = 0;
	m_decoDY = 0;	

	m_face.Init( gamui, atomUpEnabled, true );
	m_face.SetSlice( true );

	m_deco.Init( gamui, decoEnabled, false );	// does nothing; we set the level to deco
	m_deco.SetLevel( Gamui::LEVEL_DECO );

	RenderAtom nullAtom;
	m_icon.Init( gamui, nullAtom, false );
	m_icon.SetLevel( Gamui::LEVEL_ICON );

	m_label[0].Init( gamui );
	m_label[1].Init( gamui );
	gamui->Add( this );
}


float Button::Width() const
{
	float x0 = m_face.X() + m_face.Width();
	float x1 = m_deco.X() + m_deco.Width();
	float x2 = m_label[0].X() + m_label[0].Width();
	float x3 = x2;
	if ( m_usingText1 ) 
		x3 = m_label[1].X() + m_label[1].Width();

	float x = Max( x0, Max( x1, Max( x2, x3 ) ) );
	return x - X();
}


float Button::Height() const
{
	float y0 = m_face.Y() + m_face.Height();
	float y1 = m_deco.Y() + m_deco.Height();
	float y2 = m_label[0].Y() + m_label[0].Height();
	float y3 = y2;
	if ( m_usingText1 ) 
		y3 = m_label[1].Y() + m_label[1].Height();

	float y = Max( y0, Max( y1, Max( y2, y3 ) ) );
	return y - Y();
}


void Button::PositionChildren()
{
	// --- deco ---
	if ( m_face.Width() > m_face.Height() ) {
		m_deco.SetSize( m_face.Height() , m_face.Height() );

		if ( m_decoLayout == LEFT ) {
			m_deco.SetPos( X() + m_decoDX, Y() + m_decoDY );
		}
		else if ( m_decoLayout == RIGHT ) {
			m_deco.SetPos( X() + m_face.Width() - m_face.Height() + m_decoDX, Y() + m_decoDY );
		}
		else {
			m_deco.SetPos( X() + (m_face.Width()-m_face.Height())*0.5f + m_decoDX, Y() + m_decoDY );
		}
	}
	else {
		m_deco.SetSize( m_face.Width() , m_face.Width() );
		m_deco.SetPos( X() + m_decoDX, Y() + (m_face.Height()-m_face.Width())*0.5f + m_decoDY );
	}

	// --- face ---- //
	float x=0, y0=0, y1=0;

	if ( !m_usingText1 ) {
		float h = m_label[0].Height();
		m_label[1].SetVisible( false );
		y0 = Y() + (m_face.Height() - h)*0.5f;
	}
	else {
		float h0 = m_label[0].Height();
		float h2 = m_label[1].Height();
		float h = h0 + h2;

		y0 = Y() + (m_face.Height() - h)*0.5f;
		y1 = y0 + h0;

		m_label[1].SetVisible( Visible() );
	}

	// --- icon -- //
	float iconSize = Min( m_face.Height(), m_face.Width() ) * 0.5f;
	m_icon.SetSize( iconSize, iconSize );
	m_icon.SetPos( m_face.X() + m_face.Width() - iconSize, m_face.Y() + m_face.Height() - iconSize );

	// --- text --- //

	float w = m_label[0].Width();
	if ( m_usingText1 ) {
		w = Max( w, m_label[1].Width() );
	}
	
	if ( m_textLayout == LEFT ) {
		x = X();
	}
	else if ( m_textLayout == RIGHT ) {
		x = X() + m_face.Width() - w;
	}
	else {
		x = X() + (m_face.Width()-w)*0.5f;
	}

	x += m_textDX;
	y0 += m_textDY;
	y1 += m_textDY;

	m_label[0].SetPos( x, y0 );
	m_label[1].SetPos( x, y1 );

	m_label[0].SetVisible( Visible() );
	m_deco.SetVisible( Visible() );
	m_face.SetVisible( Visible() );
	m_icon.SetVisible( Visible() );

	// Modify(); don't call let sub-functions check
}


void Button::SetPos( float x, float y )
{
	UIItem::SetPos( x, y );
	m_face.SetPos( x, y );
	m_deco.SetPos( x, y );
	m_icon.SetPos( x, y );
	m_label[0].SetPos( x, y );
	m_label[1].SetPos( x, y );
	// Modify(); don't call let sub-functions check
}

void Button::SetSize( float width, float height )
{
	m_face.SetSize( width, height );
	float size = Min( width, height );
	m_deco.SetSize( size, size );
	m_icon.SetSize( size*0.5f, size*0.5f );
	// Modify(); don't call let sub-functions check
}


void Button::SetText( const char* text )
{
	const char* p = strchr( text, '\n' );
	if ( p && *(p+1) ) {
		m_label[0].SetText( text, p );	// calls Modify()
		SetText2( p+1 );
	}
	else {
		m_label[0].SetText( text );	// calls Modify()
	}
}


void Button::SetText2( const char* text )
{
	m_usingText1 = true;
	m_label[1].SetText( text );
	Modify();
}


void Button::SetState()
{
	int faceIndex = UP;
	int decoIndex = DECO;
	int iconIndex = ICON;
	if ( m_enabled ) {
		if ( m_up ) {
			// defaults set
		}
		else {
			faceIndex = DOWN;
		}
	}
	else {
		if ( m_up ) {
			faceIndex = UP_D;
			decoIndex = DECO_D;
			iconIndex = ICON_D;
		}
		else {
			faceIndex = DOWN_D;
			decoIndex = DECO_D;
			iconIndex = ICON_D;
		}
	}
	m_face.SetAtom( m_atoms[faceIndex] );
	m_deco.SetAtom( m_atoms[decoIndex] );
	m_icon.SetAtom( m_atoms[iconIndex] );
	m_label[0].SetEnabled( m_enabled );
	m_label[1].SetEnabled( m_enabled );
	//Modify(); sub functions should call
}


void Button::SetEnabled( bool enabled )
{
	m_enabled = enabled;
	SetState();
}


const RenderAtom* Button::GetRenderAtom() const
{
	return 0;
}


bool Button::DoLayout()
{
	PositionChildren();
	return false;	// children render, not this
}


void Button::Queue( CDynArray< uint16_t > *indexBuf, CDynArray< Gamui::Vertex > *vertexBuf )
{
	// does nothing - children draw
}


bool PushButton::HandleTap( TapAction action, float x, float y )
{
	bool activated = false;
	if ( action == TAP_DOWN ) {
		m_up = false;
		activated = true;
	}
	else if ( action == TAP_UP || action == TAP_CANCEL ) {
		m_up = true;
		if ( action == TAP_UP && x >= X() && x < X()+Width() && y >= Y() && y < Y()+Height() ) {
			activated = true;
		}
	}
	SetState();
	return activated;
}


void ToggleButton::Clear()
{
	RemoveFromToggleGroup();
	Button::Clear();
}


void ToggleButton::RemoveFromToggleGroup()
{
	if ( m_next ) {
		ToggleButton* other = m_next;
		
		// unlink
		m_prev->m_next = m_next;
		m_next->m_prev = m_prev;

		// clean up the group just left.
		if ( other != this ) {
			other->ProcessToggleGroup();
		}
		m_next = m_prev = this;
	}
}


void ToggleButton::AddToToggleGroup( ToggleButton* button )
{
	// Used to assert, but easy mistake and sometimes useful to not have to check.
	if ( button == this )
		return;
	GAMUIASSERT( this->m_next && button->m_next );
	if ( this->m_next && button->m_next ) {

		button->RemoveFromToggleGroup();
		
		button->m_prev = this;
		button->m_next = this->m_next;

		this->m_next->m_prev = button;
		this->m_next = button;

		ProcessToggleGroup();
	}
}


bool ToggleButton::InToggleGroup()
{
	return m_next != this;
}


void ToggleButton::ProcessToggleGroup()
{
	if ( m_next && m_next != this ) {
		// One and only one button can be down.
		ToggleButton* firstDown = 0;
		ToggleButton* candidate = 0;

		if ( this->Down() )
			firstDown = this;

		for( ToggleButton* it = this->m_next; it != this; it = it->m_next ) {
			if ( firstDown )
				it->PriSetUp();
			else if ( it->Down() && it->Visible() && it->Enabled() )
				firstDown = it;
			else
				it->PriSetUp();

			if ( !firstDown && it->Visible() && it->Enabled() )
				candidate = it;
		}

		if ( !firstDown && Visible() && Enabled() )
			this->PriSetDown();
		else if ( candidate )
			candidate->PriSetDown();
		else
			this->PriSetDown();
	}
}


bool ToggleButton::HandleTap( TapAction action, float x, float y )
{
	bool activated = false;
	if ( action == TAP_DOWN ) {
		m_wasUp = m_up;
		m_up = false;
		activated = true;
	}
	else if ( action == TAP_UP || action == TAP_CANCEL ) {
		m_up = m_wasUp;
		if ( action == TAP_UP && x >= X() && x < X()+Width() && y >= Y() && y < Y()+Height() ) {
			activated = true;
			m_up = !m_wasUp;
			ProcessToggleGroup();
		}
	}
	SetState();
	return activated;
}


DigitalBar::DigitalBar() : UIItem( Gamui::LEVEL_FOREGROUND ),
	SPACING( 0.15f ),
	m_nTicks( 0 ),
	m_width( DEFAULT_SIZE ),
	m_height( DEFAULT_SIZE )
{
}


void DigitalBar::Init(	Gamui* gamui,
						int nTicks,
						const RenderAtom& atom0,
						const RenderAtom& atom1,
						const RenderAtom& atom2 )
{
	m_gamui = gamui;
	m_gamui->Add( this );

	GAMUIASSERT( nTicks <= MAX_TICKS );
	m_nTicks = nTicks;
	m_t0 = 0;
	m_t1 = 0;

	m_atom[0] = atom0;
	m_atom[1] = atom1;
	m_atom[2] = atom2;

	for( int i=0; i<nTicks; ++i ) {
		m_image[i].Init( gamui, atom0, true );
	}
	SetRange( 0, 0 );
}


void DigitalBar::SetSize( float w, float h )
{
	m_width = w;
	m_height = h;
	Modify();
}


void DigitalBar::SetRange( float t0, float t1 )
{
	if ( t0 != m_t0 || t1 != m_t1 ) {
		if ( t0 < 0 ) t0 = 0;
		if ( t0 > 1 ) t1 = 1;
		if ( t1 < 0 ) t1 = 0;
		if ( t1 > 1 ) t1 = 1;

		t1 = Max( t1, t0 );

		m_t0 = t0;
		m_t1 = t1;

		int index0 = (int)( t0 * (float)m_nTicks + 0.5f );
		int index1 = (int)( t1 * (float)m_nTicks + 0.5f );

		for( int i=0; i<index0; ++i ) {
			m_image[i].SetAtom( m_atom[0] );
		}
		for( int i=index0; i<index1; ++i ) {
			m_image[i].SetAtom( m_atom[1] );
		}
		for( int i=index1; i<m_nTicks; ++i ) {
			m_image[i].SetAtom( m_atom[2] );
		}
		Modify();
	}
}


float DigitalBar::Width() const
{
	return m_width;
}


float DigitalBar::Height() const
{
	return m_height;
}


void DigitalBar::SetVisible( bool visible )
{
	UIItem::SetVisible( visible );
	for( int i=0; i<m_nTicks; ++i ) {
		m_image[i].SetVisible( visible );
	}
}


const RenderAtom* DigitalBar::GetRenderAtom() const
{
	return 0;
}


bool DigitalBar::DoLayout()
{
	float perItemWidth = m_width*(1.0f-SPACING) / (float)m_nTicks;
	float space = m_width*SPACING / (float)m_nTicks;

	for( int i=0; i<m_nTicks; ++i ) {
		m_image[i].SetSize( perItemWidth, m_height );
		m_image[i].SetPos( X() + (float)i*perItemWidth + (float)i*space,
						   Y() );
	}
	return false;
}


void DigitalBar::Queue( CDynArray< uint16_t > *indexBuf, CDynArray< Gamui::Vertex > *vertexBuf )
{
	// does nothing - children draw
}


Gamui::Gamui()
	:	m_itemTapped( 0 ),
		m_iText( 0 ),
		m_orderChanged( true ),
		m_modified( true ),
		m_itemArr( 0 ),
		m_nItems( 0 ),
		m_nItemsAllocated( 0 ),
		m_dragStart( 0 ),
		m_dragEnd( 0 ),
		m_textHeight( 16 ),
		m_focus( -1 ),
		m_focusImage( 0 )
{
}


Gamui::Gamui(	IGamuiRenderer* renderer,
				const RenderAtom& textEnabled, 
				const RenderAtom& textDisabled,
				IGamuiText* iText ) 
	:	m_itemTapped( 0 ),
		m_iText( 0 ),
		m_orderChanged( true ),
		m_modified( true ),
		m_itemArr( 0 ),
		m_nItems( 0 ),
		m_nItemsAllocated( 0 ),
		m_textHeight( 16 ),
		m_focus( -1 ),
		m_focusImage( 0 )
{
	Init( renderer, textEnabled, textDisabled, iText );
}


Gamui::~Gamui()
{
	delete m_focusImage;
	for( int i=0; i<m_nItems; ++i ) {
		m_itemArr[i]->Clear();
	}
	free( m_itemArr );
}


void Gamui::Init(	IGamuiRenderer* renderer,
					const RenderAtom& textEnabled, 
					const RenderAtom& textDisabled,
					IGamuiText* iText )
{
	m_iRenderer = renderer;
	m_textAtomEnabled = textEnabled;
	m_textAtomDisabled = textDisabled;
	m_iText = iText;
}


void Gamui::Add( UIItem* item )
{
	if ( m_nItemsAllocated == m_nItems ) {
		m_nItemsAllocated = m_nItemsAllocated*3/2 + 16;
		m_itemArr = (UIItem**) realloc( m_itemArr, m_nItemsAllocated*sizeof(UIItem*) );
	}
	m_itemArr[m_nItems++] = item;
	OrderChanged();
}


void Gamui::Remove( UIItem* item )
{
	// Remove from the focus list.
	for( int i=0; i<m_focusItems.Size(); ++i ) {
		if ( m_focusItems[i].item == item ) {
			m_focusItems.SwapRemove( i );
			break;
		}
	}

	// hmm...linear search. could be better.
	for( int i=0; i<m_nItems; ++i ) {
		if ( m_itemArr[i] == item ) {
			// swap off the back.
			m_itemArr[i] = m_itemArr[m_nItems-1];
			m_nItems--;

			item->Clear();
			break;
		}
	}
	OrderChanged();
}


const UIItem* Gamui::Tap( float x, float y )
{
	TapDown( x, y );
	return TapUp( x, y );
}


void Gamui::TapDown( float x, float y )
{
	GAMUIASSERT( m_itemTapped == 0 );
	m_itemTapped = 0;

	for( int i=0; i<m_nItems; ++i ) {
		UIItem* item = m_itemArr[i];

		if (	item->CanHandleTap()    
			 && item->Enabled() 
			 && item->Visible()
			 && x >= item->X() && x < item->X()+item->Width()
			 && y >= item->Y() && y < item->Y()+item->Height() )
		{
			if ( item->HandleTap( UIItem::TAP_DOWN, x, y ) ) {
				m_itemTapped = item;
				break;
			}
		}
	}
}


const UIItem* Gamui::TapUp( float x, float y )
{
	m_dragStart = m_itemTapped;

	const UIItem* result = 0;
	if ( m_itemTapped ) {
		if ( m_itemTapped->HandleTap( UIItem::TAP_UP, x, y ) )
			result = m_itemTapped;
	}
	m_itemTapped = 0;

	m_dragEnd = 0;
	for( int i=0; i<m_nItems; ++i ) {
		UIItem* item = m_itemArr[i];

		if (    item->CanHandleTap()
			 &&	item->Enabled() 
			 && item->Visible()
			 && x >= item->X() && x < item->X()+item->Width()
			 && y >= item->Y() && y < item->Y()+item->Height() )
		{
			m_dragEnd = item;
			break;
		}
	}
	return result;
}


void Gamui::TapCancel()
{
	if ( m_itemTapped ) {
		m_itemTapped->HandleTap( UIItem::TAP_CANCEL, 0, 0 );
	}
	m_itemTapped = 0;

}

int Gamui::SortItems( const void* _a, const void* _b )
{ 
	const UIItem* a = *((const UIItem**)_a);
	const UIItem* b = *((const UIItem**)_b);
	// Priorities:
	// 1. Level
	// 2. RenderState
	// 3. Texture

	// Level wins.
	if ( a->Level() < b->Level() )
		return -1;
	else if ( a->Level() > b->Level() )
		return 1;

	const RenderAtom* atomA = a->GetRenderAtom();
	const RenderAtom* atomB = b->GetRenderAtom();

	const void* rStateA = (atomA) ? atomA->renderState : 0;
	const void* rStateB = (atomB) ? atomB->renderState : 0;

	// If level is the same, look at the RenderAtom;
	// to get to the state:
	if ( rStateA < rStateB )
		return -1;
	else if ( rStateA > rStateB )
		return 1;

	const void* texA = (atomA) ? atomA->textureHandle : 0;
	const void* texB = (atomB) ? atomB->textureHandle : 0;

	// finally the texture.
	if ( texA < texB )
		return -1;
	else if ( texA > texB )
		return 1;

	// just to guarantee a consistent order.
	return a - b;
}


void Gamui::Render()
{
	if ( m_focusImage ) {
		const UIItem* focused = GetFocus();
		if ( focused ) {
			m_focusImage->SetVisible( true );
			m_focusImage->SetSize( GetTextHeight()*2.5f, GetTextHeight()*2.5f );
			m_focusImage->SetCenterPos( focused->X() + focused->Width()*0.5f, focused->Y() + focused->Height()*0.5f );
		}
		else {
			m_focusImage->SetVisible( false );
		}
	}

	if ( m_orderChanged ) {
		qsort( m_itemArr, m_nItems, sizeof(UIItem*), SortItems );
		m_orderChanged = false;
	}

	if ( m_modified ) {
		State* state = 0;

		m_stateBuffer.Clear();
		m_indexBuffer.Clear();
		m_vertexBuffer.Clear();

		for( int i=0; i<m_nItems; ++i ) {
			UIItem* item = m_itemArr[i];
			const RenderAtom* atom = item->GetRenderAtom();

			// Requires() does layout / sets child visibility. Can't skip this step:
			bool needsToRender = item->DoLayout();

			if ( !needsToRender || !item->Visible() || !atom || !atom->textureHandle )
				continue;

			// Do we need a new state?
			if (    !state
				 || atom->renderState   != state->renderState
				 || atom->textureHandle != state->textureHandle ) 
			{
				state = m_stateBuffer.PushArr( 1 );
				state->vertexStart = m_vertexBuffer.Size();
				state->indexStart = m_indexBuffer.Size();
				state->renderState = atom->renderState;
				state->textureHandle = atom->textureHandle;
			}
			item->Queue( &m_indexBuffer, &m_vertexBuffer );

			state->nVertex = (uint16_t)m_vertexBuffer.Size() - state->vertexStart;
			state->nIndex  = (uint16_t)m_indexBuffer.Size() - state->indexStart;
		}
		m_modified = false;
	}

	if ( m_indexBuffer.Size() >= 65536 || m_vertexBuffer.Size() >= 65536 ) {
		GAMUIASSERT( 0 );
		return;
	}

	const void* renderState = 0;
	const void* textureHandle = 0;

	m_iRenderer->BeginRender();
	for( int i=0; i<m_stateBuffer.Size(); ++i ) {
		const State& state = m_stateBuffer[i];

		if ( state.renderState != renderState ) {
			m_iRenderer->BeginRenderState( state.renderState );
			renderState = state.renderState;
		}
		if ( state.textureHandle != textureHandle ) {
			m_iRenderer->BeginTexture( state.textureHandle );
			textureHandle = state.textureHandle;
		}

		m_iRenderer->Render(	renderState, 
								textureHandle, 
								state.nIndex, &m_indexBuffer[state.indexStart], 
								state.nVertex, &m_vertexBuffer[0] );
	}
	m_iRenderer->EndRender();
}


void Gamui::Layout( UIItem** item, int nItems,
					int cx, int cy,
					float originX, float originY,
					float tableWidth, float tableHeight )
{
	float itemWidth = 0, itemHeight = 0;
	if ( nItems == 0 )
		return;

	for( int i=0; i<cx && i<nItems; ++i )
		itemWidth += item[i]->Width();
	for( int i=0; i<cy && (i*cx)<nItems; ++i )
		itemHeight += item[i*cx]->Height();

	float xSpacing = 0;
	if ( cx > 1 ) { 
		xSpacing = ( tableWidth - itemWidth ) / (float)(cx-1);
	}
	float ySpacing = 0;
	if ( cy > 1 ) {
		ySpacing = ( tableHeight - itemHeight ) / (float)(cy-1);
	}

	int c = 0;

	float y = originY;
	for( int j=0; j<cy && c<nItems; ++j ) {
		float x=originX;
		for( int i=0; i<cx && c<nItems; ++i ) {
//			if ( i > 0 && i==(cx-1) ) {
//				x = originX+tableWidth-item[c]->Width();	// be extra careful with rounding.
//			}
//			if ( j > 0 && j==(cy-1) ) {
//				y = originY+tableHeight-item[c]->Height();	// be extra careful with rounding.
//			}

			item[c]->SetPos( x, y );
			x += item[c]->Width();
			x += xSpacing;
			++c;
		}
		y += item[c-1]->Height();
		y += ySpacing;
	}
}


void Gamui::Layout( UIItem** item, int nItems,
					int columns,
					float originX, float originY )
{
	int c = 0;
	float x = originX;
	float y = originY;

	for( int i=0; i<nItems; ++i ) {
		item[i]->SetPos( x, y );
		x += item[i]->Width();

		++c;
		if ( c >= columns ) {
			c = 0;
			x = originX;
			y += item[i]->Height();
		}
	}
}


const char* SkipSpace( const char* p ) {
	while( p && *p && *p == ' ' ) {
		++p;
	}
	return p;
}

const char* EndOfWord( const char* p ) {
	while( p && *p && *p != ' ' && *p != '\n' ) {
		++p;
	}
	return p;
}


void Gamui::LayoutTextBlock(	const char* text,
								TextLabel* textLabels, int nText,
								float originX, float originY,
								float width )
{
	GAMUIASSERT( text );
	const char* p = text;
	int i = 0;

	TextLabel label;
	label.Init( this );
	label.SetText( "X" );
	float lineHeight = label.Height();

	while ( i < nText && *p ) {
		label.ClearText();
		
		// Add first word: always get at least one.		
		p = SkipSpace( p );
		const char* end = EndOfWord( p );
		end = SkipSpace( end );

		// (p,end) definitely gets added. The question is how much more?
		const char* q = end;
		while ( *q && *q != '\n' ) {
			q = EndOfWord( q );
			q = SkipSpace( q );
			label.SetText( p, q );
			if ( label.Width() > width ) {
				break;
			}
			else {
				end = q;
			}
		}
		
		textLabels[i].SetText( p, end );
		textLabels[i].SetPos( originX, originY + (float)i*lineHeight );
		p = end;
		++i;
		// We just put in a carriage return, so the first is free:
		if ( *p == '\n' )
			++p;

		// The rest advance i:
		while ( *p == '\n' && i < nText ) {
			textLabels[i].ClearText();
			++i;
			++p;
		}
	}
	while( i < nText ) {
		textLabels[i].ClearText();
		++i;
	}
}


void Gamui::AddToFocusGroup( const UIItem* item, int id )
{
	FocusItem* fi = m_focusItems.PushArr(1);
	fi->item = item;
	fi->group = id;
}


void Gamui::SetFocus( const UIItem* item )
{
	m_focus = -1;
	for( int i=0; i<m_focusItems.Size(); ++i ) {
		if ( m_focusItems[i].item == item ) {
			m_focus = i;
			break;
		}
	}
}


const UIItem* Gamui::GetFocus() const
{
	if ( m_focus >= 0 && m_focus < m_focusItems.Size() ) {
		return m_focusItems[m_focus].item;
	}
	return 0;
}


float Gamui::GetFocusX()
{
	const UIItem* item = GetFocus();
	if ( item ) {
		return item->CenterX();
	}
	return -1;
}


float Gamui::GetFocusY()
{
	const UIItem* item = GetFocus();
	if ( item ) {
		return item->CenterY();
	}
	return -1;
}


void Gamui::SetFocusLook( const RenderAtom& atom, float zRotation )
{
	if ( !m_focusImage ) {
		m_focusImage = new Image( this, atom, true );
		m_focusImage->SetLevel( LEVEL_FOCUS );
	}
	m_focusImage->SetAtom( atom );
	m_focusImage->SetRotationZ( zRotation );
}


void Gamui::MoveFocus( float x, float y )
{
	if ( m_focusItems.Size() == 0 ) return;
	if ( m_focusItems.Size() == 1 ) SetFocus( m_focusItems[0].item );

	float bestDist = 1000000.0f;
	int bestIndex  = -1;

	const UIItem* focused = GetFocus();
	for( int i=0; i<m_focusItems.Size(); ++i ) {
		const UIItem* item = m_focusItems[i].item;
		if ( item == focused ) {
			continue;
		}
		if ( !item->Enabled() || !item->Visible() ) {
			continue; 
		}
		float dx = item->CenterX() - focused->CenterX();
		float dy = item->CenterY() - focused->CenterY();

		float score = dx*x + dy*y;
		if ( score > 0 ) {
			float dist = sqrt( dx*dx + dy*dy );
			if ( dist < bestDist ) {
				bestDist = dist;
				bestIndex = i;
			}
		}
	}
	if ( bestIndex >= 0 ) {
		SetFocus( m_focusItems[bestIndex].item );
	}
}


LayoutCalculator::LayoutCalculator( float w, float h ) 
	: screenWidth( w ),
	  screenHeight( h ),
	  width( 10 ),
	  height( 10 ),
	  gutterX( 0 ), 
	  gutterY( 0 ), 
	  spacing( 0 ),
	  textOffsetX( 0 ),
	  textOffsetY( 0 ),
	  offsetX( 0 ),
	  offsetY( 0 ),
	  useTextOffset( false ),
	  innerX0( 0 ),
	  innerY0( 0 ),
	  innerX1( w ),
	  innerY1( h )
{
}


LayoutCalculator::~LayoutCalculator()
{
}


void LayoutCalculator::PosAbs( UIItem* item, int _x, int _y, bool setSize )
{
	float pos[2] = { 0, 0 };
	int xArr[2] = { _x, _y };
	float size[2] = { width, height };
	float screen[2] = { screenWidth, screenHeight };
	float gutter[2] = { gutterX, gutterY };

	for( int i=0; i<2; ++i ) {
		if ( xArr[i] >= 0 ) {
			float x = (float)xArr[i];	// 0 based
			float space = spacing*x;
			pos[i] = gutter[i] + space + size[i]*x;
		}
		else {
			float x = -(float)xArr[i]; // 1 based
			float space = spacing*(x-1.0f);
			pos[i] = screen[i] - gutter[i] - space - size[i]*x; 
		}
	}
	if ( useTextOffset ) {
		pos[0] += textOffsetX;
		pos[1] += textOffsetY;
	}
	item->SetPos( pos[0]+offsetX, pos[1]+offsetY );

	if ( setSize ) {
		item->SetSize( width, height );
	}

	if ( item->Visible() ) {
		// Track the inner rectangle.
		float x0, x1, y0, y1;

		if ( _x >= 0 ) {
			x0 = item->X() + item->Width() + gutter[0];
			x1 = screenWidth;
		}
		else {
			x0 = 0;
			x1 = item->X() - gutter[0];
		}
		if ( _y >= 0 ) {
			y0 = item->Y() + item->Height() + gutter[1];
			y1 = screenHeight;
		}
		else {
			y0 = 0;
			y1 = item->Y() - gutter[1];
		}
	
		innerX0 = Max( innerX0, gutter[0] );
		innerY0 = Max( innerY0, gutter[1] );
		innerX1 = Min( innerX1, screenWidth  - gutter[0] );
		innerY1 = Min( innerY1, screenHeight - gutter[1] );

		// Can trim to y or x. Which one?
		float areaX = ( Min( innerX1, x1 ) - Max( innerX0, x0 ) ) * ( innerY1 - innerY0 );
		float areaY = ( innerX1 - innerX0 ) * ( Min( innerY1, y1 ) - Max( innerY0, y0 ));
		if ( areaX > areaY ) {
			innerX0 = Max( innerX0, x0 );
			innerX1 = Min( innerX1, x1 );
		}
		else {
			innerY0 = Max( innerY0, y0 );
			innerY1 = Min( innerY1, y1 );
		}
	}
}


void LayoutCalculator::PosInner( UIItem* item, float wDivH )
{
	innerX0 = Max( innerX0, gutterX );
	innerX1 = Min( innerX1, screenWidth - gutterX );
	innerY0 = Max( innerY0, gutterY );
	innerY1 = Min( innerY1, screenHeight - gutterY );

	float dx = innerX1 - innerX0;
	float dy = innerY1 - innerY0;

	if ( wDivH == 0 ) {
		item->SetPos( innerX0, innerY0 );
		item->SetSize( dx, dy );
	}
	else {
		if ( wDivH > (dx/dy) ) {
			float cy = dx / wDivH;
			item->SetPos( innerX0, innerY0 + (dy-cy)*0.5f );
			item->SetSize( dx, cy );
		}
		else {
			float cx = dy * wDivH;
			item->SetPos( innerX0 + (dx-cx)*0.5f, innerY0 );
			item->SetSize( cx, dy );
		}
	}
}

