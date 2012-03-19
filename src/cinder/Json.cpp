/*
 Copyright (c) 2012, The Cinder Project
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/


#include "boost/algorithm/string.hpp"
#include "json/json.h"

#include "cinder/Json.h"
#include "cinder/Stream.h"
#include "cinder/Utilities.h"

using namespace std;

namespace cinder {

JsonTree::JsonTree()
{
    init( "", Json::Value( Json::nullValue ), true, NODE_NULL );
}

JsonTree::JsonTree( const JsonTree &jsonTree )
	: mParent( NULL )
{
	mKey = jsonTree.mKey;
	mNodeType = jsonTree.mNodeType;
	mValue = jsonTree.mValue;
	mValueType = jsonTree.mValueType;

	for( ConstIter childIt = jsonTree.begin(); childIt != jsonTree.end(); ++childIt ) {
		pushBack( *childIt );
    }
}

JsonTree& JsonTree::operator=( const JsonTree &jsonTree )
{
	mParent = NULL;
	mKey = jsonTree.mKey;
	mNodeType = jsonTree.mNodeType;
	mValue = jsonTree.mValue;
	mValueType = jsonTree.mValueType;

	for( ConstIter childIt = jsonTree.begin(); childIt != jsonTree.end(); ++childIt ) {
		pushBack( *childIt );
    }

	return *this;
}

JsonTree::JsonTree( DataSourceRef dataSource )
{    
	string jsonString = loadString( dataSource );
	Json::Value value = deserializeNative( jsonString );
	if( value.isNull() ) {
		throw ExcJsonParserError();
	}
	init( "", value, true, NODE_OBJECT );
}

JsonTree::JsonTree( const std::string &jsonString )
{
    try {
		Json::Reader reader;
		Json::Value value;
		reader.parse( jsonString, value );
		init( "", value, true, NODE_OBJECT );
	}
	catch ( ... ) {
		throw ExcJsonParserError();
    }
}

JsonTree::JsonTree( const std::string &key, const Json::Value &value )
{
	init( key, value, true, NODE_VALUE );
}
    
JsonTree::JsonTree( const string &key, bool value )
{
    init( key, Json::Value( value ), false, NODE_VALUE, VALUE_BOOL );
}
    
JsonTree::JsonTree( const string &key, double value )
{
    init( key, Json::Value( value ), false, NODE_VALUE, VALUE_DOUBLE );
}
    
JsonTree::JsonTree( const string &key, float value )
{
    init( key, Json::Value( (double)value ), false, NODE_VALUE, VALUE_DOUBLE );
}

JsonTree::JsonTree( const string &key, int value )
{
    init( key, Json::Value( value ), false, NODE_VALUE, VALUE_INT );
}

JsonTree::JsonTree( const string &key, const std::string &value )
{
    init( key, Json::Value( value ), false, NODE_VALUE, VALUE_STRING );
}

JsonTree::JsonTree( const std::string &key, const char *value )
{
    init( key, Json::Value( value ), false, NODE_VALUE, VALUE_STRING );
}

JsonTree::JsonTree( const string &key, unsigned int value )
{
    init( key, Json::Value( value ), false, NODE_VALUE, VALUE_UINT );
}

JsonTree JsonTree::makeArray( const std::string &key )
{
	JsonTree result;
	result.mNodeType = NODE_ARRAY;
	result.mKey = key;
	return result;
}

JsonTree JsonTree::makeObject( const std::string &key )
{
	JsonTree result;
	result.mNodeType = NODE_OBJECT;
	result.mKey = key;	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void JsonTree::init( const string &key, const Json::Value &value, bool setType, NodeType nodeType, ValueType valueType )
{
    mKey = key;
	mNodeType = nodeType;
	mParent = 0;
	mValue = "";
	mValueType = valueType;

	if( ! value.isNull() && ( value.isArray() || value.isObject() ) ) {
        if( value.isArray() ) {
            mNodeType = NODE_ARRAY;
            for ( uint32_t i = 0; i < value.size(); i++ ) {
                pushBack( JsonTree( "", value[ i ] ) );
            }
        }
		else if( value.isObject() ) {
            mNodeType = NODE_OBJECT;
            Json::Value::Members members = value.getMemberNames();
            for( Json::Value::Members::const_iterator memberIt = members.begin(); memberIt != members.end(); ++memberIt ) {
				string key = *memberIt;
                pushBack( JsonTree( key, value[ key ] ) );
            }
        }
    }
	else {
		if( value.isBool() ) {
			mValue = toString( value.asBool() );
			if( setType ) {
				mValueType = VALUE_BOOL;
			}
		}
		else if ( value.isDouble() ) { 
			mValue = toString( value.asDouble() );
			if ( setType ) {
				mValueType = VALUE_DOUBLE;
			}
		}
		else if ( value.isInt() ) { 
			mValue = toString( value.asInt() );
			if ( setType ) {
				mValueType = VALUE_INT;
			}
		}
		else if ( value.isString() ) { 
			mValue = toString( value.asString() );
			if ( setType ) {
				mValueType = VALUE_STRING;
			}
		}
		else if ( value.isUInt() ) { 
			mValue = toString( value.asUInt() );
			if ( setType ) {
				mValueType = VALUE_UINT;
			}
		}
	}
}

//! Converts a JSON string into a JsonCpp object
Json::Value JsonTree::deserializeNative( const string &jsonString )
{
    try {
        Json::Reader reader;
        Json::Value value;
        reader.parse( jsonString, value );
        return value;
    }
	catch ( ... ) {
        throw ExcJsonParserError();
    }
}

void JsonTree::pushBack( const JsonTree &newChild )
{
	if( newChild.getKey() == "" )
		mNodeType = NODE_ARRAY;
	else
		mNodeType = NODE_OBJECT;

	mChildren.push_back( newChild );
	mChildren.back().mParent = this;
    mValue = "";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonTree& JsonTree::operator[]( const std::string &relativePath )
{
	return getChild( relativePath );
}

const JsonTree&	JsonTree::operator[]( const std::string &relativePath ) const
{
	return getChild( relativePath );
}

JsonTree& JsonTree::operator[]( uint32_t index )
{
	return getChild( index );
}

const JsonTree&	JsonTree::operator[]( uint32_t index ) const
{
	return getChild( index );
}

JsonTree::Iter JsonTree::begin() 
{ 
	return mChildren.begin(); 
}

JsonTree::ConstIter JsonTree::begin() const 
{ 
	return mChildren.begin();
}

JsonTree::Iter JsonTree::end() 
{ 
	return mChildren.end();
}

JsonTree::ConstIter JsonTree::end() const 
{ 
	return mChildren.end(); 
}

JsonTree& JsonTree::getChild( const std::string &relativePath, bool caseSensitive, char separator ) 
{
	JsonTree *child = getNodePtr( relativePath, caseSensitive, separator );
	if ( child ) {
		return *child;
	} else {
		throw ExcChildNotFound( *this, relativePath );
	}
}

const JsonTree& JsonTree::getChild( const std::string &relativePath, bool caseSensitive, char separator ) const 
{
	JsonTree *child = getNodePtr( relativePath, caseSensitive, separator );
	if ( child ) {
		return *child;
	} else {
		throw ExcChildNotFound( *this, relativePath );
	}
}

JsonTree& JsonTree::getChild( uint32_t index ) 
{
	JsonTree *child = getNodePtr( toString( index ), false, '.' );
	if ( child ) {
		return *child;
	} else {
		throw ExcChildNotFound( *this, toString( index ) );
	}
}

const JsonTree& JsonTree::getChild( uint32_t index ) const 
{
	JsonTree *child = getNodePtr( toString( index ), false, '.' );
	if ( child ) {
		return *child;
	} else {
		throw ExcChildNotFound( *this, toString( index ) );
	}
}

const list<JsonTree>& JsonTree::getChildren() const 
{ 
	return mChildren; 
}

bool JsonTree::hasChild( const string &relativePath, bool caseSensitive, char separator ) const
{
	return getNodePtr( relativePath, caseSensitive, separator ) != 0;
}

bool JsonTree::hasChildren() const
{
	return mChildren.size() > 0;
}

JsonTree& JsonTree::getParent() 
{ 
	return *mParent; 
}
const JsonTree& JsonTree::getParent() const 
{ 
	return *mParent;
}
bool JsonTree::hasParent() const 
{ 
	return mParent != 0; 
}
    
const string& JsonTree::getKey() const 
{ 
    return mKey;
}

string JsonTree::getPath( char separator ) const
{
    string result;
    
    const JsonTree *node = this;
	bool prevWasArrayIndex = false;
    while( node != 0 ) {
		bool isArrayIndex = false;
        string nodeName = node->mKey;
		if( nodeName.empty() && node->mParent ) { // should be an array index; find out which by searching our parent's children for ourselves
			size_t index = 0;
			for( ConstIter parentChildIt = node->mParent->mChildren.begin(); parentChildIt != node->mParent->mChildren.end(); ++parentChildIt, ++index ) {
				if( &(*parentChildIt) == node ) {
					isArrayIndex = true;
					break;
				}
			}
			if( isArrayIndex )
				nodeName = '[' + toString( index ) + ']';
		}
		if( (! prevWasArrayIndex) && (! nodeName.empty()) && ( node != this ) ) {
			result = nodeName + separator + result;
		} else if( ! nodeName.empty() ) {
			result = nodeName + result;
        }
        node = node->mParent;
		prevWasArrayIndex = isArrayIndex;
    }
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! Find pointer to node at specified path
JsonTree* JsonTree::getNodePtr( const string &relativePath, bool caseSensitive, char separator ) const
{
    // Format path into dotted address
	std::string path = boost::replace_all_copy( relativePath, "[", std::string( 1, separator ) );
	path = boost::replace_all_copy( path, "'", "");
	path = boost::replace_all_copy( path, "]", "");

    // Start search from this node
	JsonTree *curNode = const_cast<JsonTree*>( this );
    
    // Split address at dot and iterate tokens
	vector<string> pathComponents = split( path, separator );
	for( vector<string>::const_iterator pathIt = pathComponents.begin(); pathIt != pathComponents.end(); ++pathIt ) {
        // Declare target node
		ConstIter node;

        // The key is numeric
		if( isIndex( *pathIt ) ) {
            // Find child which uses this index as its key
			uint32_t index = boost::lexical_cast<int32_t>( *pathIt );
			uint32_t i = 0;
			for ( node = curNode->getChildren().begin(); node != curNode->getChildren().end(); ++node, i++ ) {
				if ( i == index ) {
					break;
				}
			}
		} else {	
            // Iterate children
            node = curNode->getChildren().begin();
            while( node != curNode->getChildren().end() ) {  
                // Compare child's key to path component
                bool keysMatch = false;
                string key1 = node->getKey();
                string key2 = *pathIt;
                if( caseSensitive && key1 == key2 ) {
                    keysMatch = true;
                } else if ( !caseSensitive && ( boost::iequals( key1, key2 ) ) ) {
                    keysMatch = true;
                }
                
                // Break if found, advance node if not
                if( keysMatch ) {
                    break;
                } else {
                    ++node;
                }
                
            }
            
		}

        // Return null pointer if we're out of nodes to search, 
        // otherwise assign node and continue to search its children
		if( node == curNode->getChildren().end() ) {
            return 0;
        } else {
			curNode = const_cast<JsonTree*>( &( *node ) );
        }
	}

    // Return child
	return curNode;

}

//! Checks if key is numeric
bool JsonTree::isIndex( const string &key )
{
	char* to_convert = const_cast<char *>( key.c_str() );
	char* p = to_convert;
	errno = 0;
	unsigned long val = strtoul( to_convert, &p, 10 );
    if( val > 0L ) {
        // Prevents "unused variable" warning 
    }
	return !( errno != 0 || to_convert == p || *p != 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

Json::Value JsonTree::createNativeDoc( bool createDocument ) const
{
	// Create JsonCpp value
	Json::Value value( Json::nullValue );

	// Key on node type
    switch( mNodeType ) {
		case NODE_ARRAY: {
			uint32_t i = 0;
			
			// Add children to array as objects
			for ( ConstIter childIt = mChildren.begin(); childIt != mChildren.end(); ++childIt, i++ ) {
				value[ i ] = childIt->createNativeDoc();
			}
		}
		break;
		case NODE_OBJECT:
			// Add children as value members
			for ( ConstIter childIt = mChildren.begin(); childIt != mChildren.end(); ++childIt ) {
				value[ childIt->getKey() ] = childIt->createNativeDoc();
			}
		break;
		case NODE_VALUE:
			// Set value with native data type
			switch ( mValueType ) {
			case VALUE_BOOL:
				value = Json::Value( fromString<bool>( mValue ) );
				break;
			case VALUE_DOUBLE:
				value = Json::Value( fromString<double>( mValue ) );
				break;
			case VALUE_INT:
				value = Json::Value( fromString<int32_t>( mValue ) );
				break;
			case VALUE_STRING:
				value = Json::Value( mValue );
				break;
			case VALUE_UINT:
				value = Json::Value( fromString<uint32_t>( mValue ) );
				break;
			}
		break;
		default:
        break;
    }
    
	// Return JsonCpp object
    if ( createDocument && !value.isNull() ) { 
        Json::Value doc( Json::objectValue );
        doc[ mKey ] = value;
        return doc;
    } else {
        return value;
    }
}

string JsonTree::serialize() const
{	
    stringstream ss;
    ss << *this;
    return ss.str();
}

string JsonTree::serializeNative( const Json::Value & value )
{	
	try {
		Json::StyledWriter writer;
		return writer.write( value );
	} catch ( ... ) {
		throw ExcJsonParserError();
	}
}

void JsonTree::write( const fs::path &path, bool createDocument )
{
	write( writeFile( path ), createDocument );
}

void JsonTree::write( DataTargetRef target, bool createDocument )
{
	// Declare output string
	string jsonString = "";

	try {
		
		// Create JsonCpp data to send to parser
		Json::Value value = createNativeDoc( createDocument );

		// This routine serializes JsonCpp data and formats it
		Json::StyledWriter writer;
		jsonString = writer.write( value.toStyledString() );
		boost::replace_all( jsonString, "\\n", "\r\n" );
		boost::replace_all( jsonString, "\\\"", "\"" );
		if( jsonString.length() >= 3 ) {
			jsonString = jsonString.substr( 1, boost::trim_copy( jsonString ).length() - 2 );
		}
		jsonString += "\0";
	}
	catch ( ... ) {
		throw ExcJsonParserError();
	}

	// Save data to file
	OStreamRef os = target->getStream();
	os->writeData( jsonString.c_str(), jsonString.length() );
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////

JsonTree::ExcChildNotFound::ExcChildNotFound( const JsonTree &node, const string &childPath ) throw()
{
	sprintf( mMessage, "Could not find child: %s for node: %s", childPath.c_str(), node.getPath().c_str() );
}

JsonTree::ExcNonConvertible::ExcNonConvertible( const JsonTree &node ) throw()
{
	sprintf( mMessage, "Unable to convert value for node: %s", node.getPath().c_str() );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

ostream& operator<<( ostream &out, const JsonTree &json )
{
    bool createDocument = json.mNodeType == JsonTree::NODE_VALUE;
    Json::Value value = json.createNativeDoc( createDocument );
	string doc = JsonTree::serializeNative( value );
	out << doc;
	return out;
}

} // namespace cinder
