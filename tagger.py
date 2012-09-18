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
		self.class_styles  = {}
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

	def LoadStyles(self, file):
		for line in open(file):
			reflect_type, reflect_class, reflect_style = line.rstrip().split('\t')
			self.class_styles[int(reflect_type)] = (reflect_class, reflect_style)

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
		all_types = {}
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
				match_types = {}
				for type, id in match[2]:
					str.append('%i.%s' % (type, id))
					all_types[type]    = 1
					match_types[type] = 1
				reflect_style = None
				reflect_class = None
				if len(match_types) == 0:
					raise Exception, 'Match found but no types?? Matched "%s"' % text
				elif len(match_types) == 1:
					one_key = match_types.keys()[0]
					if one_key in self.class_styles:
						reflect_class, reflect_style = self.class_styles[one_key]
					else:
						if len(self.class_styles):
							reflect_class, reflect_style = self.class_styles[0]
						else:
							reflect_class = 'reflect_ambigous'
							reflect_style = 'background-color: #CCFFFF !important; color:black !important;'
				else:
					match_types = match_types.keys()
					match_types.sort()
					reflect_style = self.class_styles[0][1]
					if (match_types[0] > 0 and match_types[-1] > 0):
						reflect_class = self.class_styles[0][0]
					else:
						reflect_class = 'reflect_ambigous'
				md5 = hashlib.md5()
				str = ';'.join(str)
				md5.update(str)
				key = md5.hexdigest()
				divs[key] = str
				doc.append('<span class="%s" style="%s" ' % (reflect_class, reflect_style))
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
		

if __name__ == '__main__':

	def test_simple(tagger):
		document = '<html><head></head><script> OnClick( IL5 )</script><body>IL-5 can be written as: <pre>IL5</pre>. Inter-leukin 5 is also synonymous with <strong IL5="not-tagged">IL5</strong>. The name: Cl- is the negative Chloride ion. By the way: CL2- < CL- while CL3- > 42. Testcase: <testtag name-2="IL5"></body></html>'
		for match in tagger.GetMatches(document, 'TEST', [9606, -1, -11]):
			print '%i-%i: "%s"' % (match[0], match[1],  document[match[0]:match[1]+1])
	
	def test_speed(tagger):
		doc = open('./docs/tp53.html').read()
		uri = 'http://en.wikipedia.org/wiki/TP53'
		rounds = 100
		t1 = time.clock()
		for i in range(rounds):
			tagger.GetMatches(doc, uri, [9606, -1], auto_detect=True)
		t2 = time.clock()
		print '%i rounds took %fs or %f ms/round.' % (rounds, t2-t1, 1000*float(t2-t1)/rounds)
		
	def test_p53(tagger):
		print tagger.GetHTML(open('./docs/tp53.html').read(), 'http://en.wikipedia.org/wiki/TP53', [9606, -1, -11])
		
	def test_utf8(tagger):
		doc = open('../test/REST/test_utf8.pdf').read()
		print tagger.GetMatches(doc, 'TEST', [9606])
		
	def test_clear(tagger):
		il5a = '<html><body>IL5</body></html>'
		il5b = tagger.GetHTML(il5a, None, [9606])
		il5c = tagger.GetHTML(il5b, None, [9606])
		print '[ORIGINAL HTML]:', il5a
		print
		print '[TAGGED HTML]  :', il5b
		print '[TAGGED AGAIN] :', il5c
		if il5c[:97] == il5b[:97]:
			print '[OK: <span> tag was successfully removed by C/C++ tagger.]'
		else:
			print '[ERROR: <span> tag was NOT removed by C/C++ tagger.]'

	test = {}
	test['simple']   = test_simple
	test['speed']    = test_speed
	test['p53']      = test_p53
	test['utf8']     = test_utf8
	test['clear']    = test_clear
	
	if len(sys.argv) < 8:
		sys.stderr.write("Must specify the following files:\n")
		sys.stderr.write("  1: Java header\n")
		sys.stderr.write("  2: Type-styles\n")
		sys.stderr.write("  3: Entities\n")
		sys.stderr.write("  4: Dictionary\n")
		sys.stderr.write("  5: Global allow/block\n")
		sys.stderr.write("  6: Local allow/block\n")
		sys.stderr.write("  7: Document file or one of the tests:\n")
		sys.stderr.write("Choose a test from:\n")
		sys.stderr.write("  " + ' '.join(sorted(test.keys())))
		sys.stderr.write('\n')
		sys.exit(1)
		
	tagger = Tagger()
	tagger.LoadHeaders(sys.argv[1])
	tagger.LoadStyles(sys.argv[2])  
	tagger.LoadNames(sys.argv[3], sys.argv[4])
	tagger.LoadGlobal(sys.argv[5])
	tagger.LoadLocal(sys.argv[6])
	
	file_or_test = sys.argv[7]
	
	if file_or_test in test:
		test[file_or_test](tagger)
	else:
		if os.path.exists(file_or_test):
			print tagger.GetHTML(open(file_or_test).read(), '', [9606, -22, -1, -11])
		else:
			sys.stderr.write('Unknown test function or file specified: "%s".\n' % file_or_test)
