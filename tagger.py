#!/usr/bin/env python

import os
import re
import sys
import time
import tempfile
import hashlib
import urlparse
import xml.sax.saxutils

import tagger_swig # Our Python wrapper module for the C++ tagger library.

stop_flag = 0

class Tagger:
	
	def __init__(self, java_script=None):
		self.re_head_begin = re.compile("<head[^>]*>", re.I)
		self.re_head_end   = re.compile("</head>", re.I)
		self.re_base_href  = re.compile("<base href=.+?>", re.I)
		self.re_html_end   = re.compile("(</body>)?[ \t\r\n]*(</html>)?", re.I)
		self.script        = java_script
		if self.script:
			self.script = self.script.strip()
		self.styles  = {}
		self.types = {}
		self.cpp_tagger    = tagger_swig.Tagger(False) # False: Using string dict., not serial dict.
		
	def LoadHeaders(self, file):
		if file.startswith("http://"):
			self.script = "<script src=\"%s\" type=\"text/javascript\"></script>\n" % file
		else:
			self.script = open(file).read()
			
	def LoadGlobal(self, file):
		self.cpp_tagger.load_global(file)
		
	def LoadLocal(self, file):
		self.cpp_tagger.load_local(file)

	def LoadNames(self, file1, file2):
		self.cpp_tagger.load_names(file1, file2)
	
	def SetStyles(self, styles, types={}):
		self.styles = styles
		self.types = types
	
	def PostprocessDocument(self, uri, document):
		if uri:
			match_head_begin = self.re_head_begin.search(document)
			if match_head_begin:
				match_base_ref = self.re_base_href.search(document, match_head_begin.end(0))
				if not match_base_ref:
					insert = match_head_begin.end(0)
					pre = document[:insert]
					info = urlparse.urlsplit(uri)
					href = info.scheme + "://" + info.netloc
					if href[-1] != "\t":
						href += "/"
					base = "<base href=\"%s\" />" % href
					post = document[insert:]
					document = "".join((pre, base.encode("utf8"), post))
		match_head_end = self.re_head_end.search(document)
		if match_head_end:
			insert = match_head_end.start(0)
			pre    = document[:insert]
			post   = document[insert:]
			document = ''.join((pre, self.script, post))
		return document

	def UnicodeToStr(self, unicode_or_str):
		if isinstance(unicode_or_str, unicode):
			return unicode_or_str.encode('utf8', 'replace')
		else:
			return unicode_or_str

	def AddName(self, name, type, identifier):
		self.cpp_tagger.add_name(self.UnicodeToStr(name), type, self.UnicodeToStr(identifier))
		
	def AllowName(self, name, document_id):
		self.cpp_tagger.allow_block_name(self.UnicodeToStr(name), self.UnicodeToStr(document_id), False)
		
	def BlockName(self, name, document_id):
		self.cpp_tagger.allow_block_name(self.UnicodeToStr(name), self.UnicodeToStr(document_id), True)
		
	def CheckName(self, name, type, identifier):
		return self.cpp_tagger.check_name(self.UnicodeToStr(name), type, self.UnicodeToStr(identifier))
		
	def GetMatches(self, document, document_id, entity_types, auto_detect=True, allow_overlap=False, protect_tags=True, max_tokens=5, ignore_blacklist=False):
		document_id = self.UnicodeToStr(document_id)
		
		params = tagger_swig.GetMatchesParams()
		#print dir(params.entity_types)
		#params.entity_types = entity_types
		#params.entity_types = []
		for entity_type in entity_types:
			params.add_entity_type(entity_type)
		params.auto_detect = auto_detect
		params.allow_overlap = allow_overlap
		params.protect_tags = protect_tags
		params.max_tokens = max_tokens
		params.ignore_blacklist = ignore_blacklist
		
		return self.cpp_tagger.get_matches(document, document_id, params)
		#return self.cpp_tagger.get_matches(document, document_id, entity_types, len(entity_types), auto_detect, allow_overlap, protect_tags, max_tokens, ignore_blacklist)
		
	def GetEntities(self, document, document_id, entity_types, auto_detect=True, allow_overlap=False, protect_tags=True, max_tokens=5, ignore_blacklist=False, format='xml'):
		if format == None:
			format = 'xml'
		format = format.lower()
		document_id = self.UnicodeToStr(document_id)
		matches = self.GetMatches(document, document_id, entity_types, auto_detect, allow_overlap, protect_tags, max_tokens, ignore_blacklist)
		doc = []
		if format == 'xml':
			uniq = {}
			for match in matches:
				if not match[2]:
					continue
				text = document[match[0]:match[1]+1]
				if text not in uniq:
					uniq[text] = []
				uniq[text].append(match)
			doc.append('<?xml version="1.0" encoding="UTF-8"?>')
			doc.append('<GetEntitiesResponse xmlns="Reflect" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">')
			doc.append('<items>')
			for text in uniq:
				matches = uniq[text]
				match = matches[0]
				doc.append('<item>')
				doc.append('<name xsi:type="xsd:string">%s</name>' % xml.sax.saxutils.escape(text))
				doc.append('<count xsi:type="xsd:int">%i</count>' % len(matches))
				doc.append('<entities>')
				for type, identifier in match[2]:
					doc.append('<entity>')
					doc.append('<type xsi:type="xsd:int">%i</type>' % type)
					doc.append('<identifier xsi:type="xsd:string">%s</identifier>' % identifier)
					doc.append('</entity>')
				doc.append('</entities>')
				doc.append('</item>')
			doc.append('</items>')
			doc.append('</GetEntitiesResponse>')
			doc = ''.join(doc)
			
		else:
			uniq = {}
			for match in matches:
				text = document[match[0]:match[1]+1]
				if match[2] != None:
					for type, identifier in match[2]:
						key = (text, type, str(identifier))
						if key not in uniq:
							uniq[key] = 1
			sep = '\t'
			if format == 'tsv':
				sep = '\t'
			elif format == 'csv':
				sep = ','
			elif format == 'ssv':
				sep = ';'
			for text, type, identifier in uniq:
				if sep in text:
					text = '"' + text + '"'
				if sep in identifier:
					identifier = '"' + identifier + '"'
				doc.append('%s%s%i%s%s' % (text, sep, type, sep, identifier))
			doc = '\n'.join(doc)
		return doc
	
	def GetHTML(self, document, document_id, entity_types, auto_detect=True, allow_overlap=False, protect_tags=True, max_tokens=5, ignore_blacklist=False, html_footer=''):
		document_id = self.UnicodeToStr(document_id)
		doc = []
		i = 0
		all_types = set()
		divs = {}
		matches = self.GetMatches(document, document_id, entity_types, auto_detect, allow_overlap, protect_tags, max_tokens, ignore_blacklist)
		matches.sort()
		for match in matches:
			length = match[0] - i
			if length > 0:
				doc.append(document[i:match[0]])
			if match[2] != None:
				text = document[match[0]:match[1]+1]
				str = []
				match_types = set()
				for type, id in match[2]:
					str.append('%i.%s' % (type, id))
					all_types.add(type)
					match_types.add(type)
				reflect_style = None
				for priority in self.styles:
					if priority not in self.types:
						reflect_style = self.styles[priority]
						break
					f = self.types[priority]
					for type in match_types:
						if f(type):
							reflect_style = self.styles[priority]
							break
					if reflect_style:
						break
				md5 = hashlib.md5()
				str = ';'.join(str)
				md5.update(str)
				key = md5.hexdigest()
				divs[key] = str
				doc.append('<span class="reflect_tag" style="%s" ' % reflect_style)
				doc.append('onMouseOver="startReflectPopupTimer(\'%s\',\'%s\')" ' % (key, text))
				doc.append('onMouseOut="stopReflectPopupTimer()" ')
				doc.append('onclick="showReflectPopup(\'%s\',\'%s\')">' % (key, text))
				doc.append(text)
				doc.append('</span>')
			i = match[1] + 1
		if i < len(document):
			length = len(document)-i
			doc.append(document[i:i+length+1])
		doc.append('<div class="reflect_entities" style="display: none;">\n')
		for key in divs:
			str = divs[key]
			doc.append('  <span name="%s">%s</span>\n' % (key, str))
		doc.append('</div>\n')
		doc.append('<div style="display: none;" class="reflect_entity_types">\n')
		for type in all_types:
			doc.append('  <span>%i</span>\n' % type)
		doc.append('</div>\n')
		doc.append('<div style="display: none;" id="reflect_div_doi">%s</div>\n' % document_id)
		doc.append('<div name="reflect_v2" style="display: none;"></div>\n')
		doc.append(html_footer)
		doc.append('</body>\n</html>\n')
		return self.PostprocessDocument(document_id, ''.join(doc))
