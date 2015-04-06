#! /usr/bin/python
import sys
import os
import codecs
import shutil
import copy
import xml.etree.ElementTree as ET
from bs4 import BeautifulSoup

def findCompoundName( tree ):
	for compoundDef in tree.iter( "compounddef" ):
		for compoundName in tree.iter( "compoundname" ):
			print compoundName.text
			return compoundName.text

def markupFunction( fnXml, parent, bs4 ):
	li = bs4.new_tag( "li" )
	definition = bs4.new_tag( "em" )
	definition.append( fnXml.find( "definition" ).text );
	li.append( definition )
	li.append( fnXml.find( "argsstring" ).text )
	parent.append( li )

def replaceTag( tag ):

	if( tag == "para" ):
		return "p"
	elif( tag == "linebreak" ):
		return "br"
	elif( tag == "emphasis" ):
		return "em"
	elif (tag == "ref" ):
		return "a"
	elif( tag == "computeroutput" ):
		return "code"
	else:
		return tag

def iterateMarkup( tree, parent, html ):

	origTag = tree.tag

	currentTag = parent
	
	# append any new tags
	if tree.tag != None :
		htmlTag = html.new_tag( replaceTag( tree.tag ) )
		currentTag = htmlTag
		parent.append( currentTag )
	
	# add content to tag
	if tree.text != None:
		currentTag.append( tree.text.strip() )
	
	# iterate through children tags
	for child in list( tree ):
		output = iterateMarkup( child, currentTag, html )

	# tail is any extra text that isn't wrapped in another tag
	# that exists before the next tag
	if tree.tail != None:
		parent.append( tree.tail.strip() )
	
	return currentTag

def markupParagraph( paraXml, parent, html ):

	print "\n-- MARKING UP PARAGRAPH --"
	
	description_el = iterateMarkup( paraXml, parent, html )
	parent.append( description_el )
	
	
def processClassXmlFile( inPath, outPath, html ):
	print "Processing file: " + inPath + " > " + outPath;
	tree = ET.parse( inPath )
	
	# add title
	headTag = html.head
	titleTag = html.new_tag( "title" )
	titleTag.append( "Cinder: " + str(findCompoundName( tree )) )
	headTag.insert( 0, titleTag )

	# find contents wrapper
	contentsTag = html.find( "div", "contents" )

	# description
	descriptionHeader = html.new_tag( "h3")
	descriptionHeader.string = "Description"
	contentsTag.append( descriptionHeader )

	descTag = html.new_tag( "div" )
	descTag['class'] = "description"

	markupParagraph( tree.find( r'compounddef/detaileddescription/' ), descTag, html );
	
	contentsTag.append( descTag )

	# create regular and static member function ul wrappers
	ulTag = html.new_tag( "ul" )
	staticUlTag = html.new_tag( "ul" )
	staticUlTag['class'] = "static"	

	funcCount = 0
	staticFuncCount = 0

	# public member functions
	for memberFn in tree.findall( r'compounddef/sectiondef/memberdef[@kind="function"][@prot="public"]' ):
		# split between static or not
		isStatic = memberFn.attrib[ "static" ]

		if isStatic == 'yes':
			staticFuncCount += 1
			markupFunction( memberFn, staticUlTag, html )
		else:
			funcCount += 1
			markupFunction( memberFn, ulTag, html )
			
	# if function count > 0 add it
	if funcCount > 0 :
		header = html.new_tag( "h3")
		header.string = "Member Functions"
		contentsTag.append( header )
		contentsTag.append( ulTag )

	# if static function count > 0 add it
	if staticFuncCount > 0 :
		header = html.new_tag( "h3")
		header.string = "Static Member Functions"
		contentsTag.append( header )
		contentsTag.append( staticUlTag )

	# write the file	
	outFile = codecs.open( outPath, "w", "UTF-8" )
	outFile.write( html.prettify() )

def parseTemplateHtml( templatePath ):
	file = codecs.open( templatePath, "r", "UTF-8" )
	return BeautifulSoup( file )

if __name__ == "__main__":
	htmlSourcePath = os.path.dirname( os.path.realpath(__file__) ) + os.sep + 'htmlsrc' + os.sep
	doxygenHtmlPath = os.path.dirname( os.path.realpath(__file__) ) + os.sep + 'html' + os.sep

	if len( sys.argv ) == 3: # process a specific file
		templateHtml = parseTemplateHtml( os.path.join( htmlSourcePath, "cinderClassTemplate.html" ) )
		processClassXmlFile( sys.argv[1], os.path.join( doxygenHtmlPath, sys.argv[2] ), copy.deepcopy( templateHtml ) )
	else:
		print "Unknown usage"