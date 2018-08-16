import datetime
import hashlib
import json
import os
import re
import sys
import tempfile
import threading
import time
import xml.sax.saxutils

PY3 = sys.version_info >= (3,)

if PY3:
	from urllib.parse import urlparse
else:
	import urlparse

import tagger_swig


def entity_dict(qtype, qid):
	data = {}
	if qtype >= 0:
		data = {"@id": "stringdb:%d.%s" % (qtype, qid)}
	elif qtype == -1:
		data = {"@id": "stitchdb:%s" % qid}
	elif qtype == -2:
		data = {"@id": "taxonomy:%s" % qid}
	elif ":" in qid:
		data = {"@id": qid}
	else:
		data = {"@id": "_:%s" % qid}
	return data


class Tagger:

	def __init__(self, java_script=None, re_stop=None, serials_only=False):
		self.re_head_begin = re.compile("<head[^>]*>", re.I)
		self.re_head_end   = re.compile("</head>", re.I)
		self.re_base_href  = re.compile("<base href=.+?>", re.I)
		self.re_html_end   = re.compile("(</body>)?[ \t\r\n]*(</html>)?", re.I)
		self.script        = java_script
		if self.script:
			self.script = self.script.strip()
		self.styles  = {}
		self.types = {}
		self.blocked_documents = {}
		self.blocked_documents_lock = threading.Lock()
		self.changelog_file = None
		self.changelog_lock = threading.Lock()
		self.document_types = {}
		self.document_types_lock = threading.Lock()
		if re_stop is None:
			self.cpp_tagger = tagger_swig.Tagger(serials_only)
		else:
			self.cpp_tagger = tagger_swig.Tagger(serials_only, re_stop)

	def load_changelog(self, file):
		self.changelog_lock.acquire()
		self.changelog_file = None
		if os.path.exists(file):
			for line in open(file):
				tokens = line[:-1].split('\t')
				if tokens[1] == "AddName":
					self.add_name(tokens[2], tokens[3], tokens[4])
				elif tokens[1] == "AllowName":
					self.allow_name(tokens[2], tokens[3])
				elif tokens[1] == "BlockName":
					self.block_name(tokens[2], tokens[3])
		self.changelog_file = file
		self.changelog_lock.release()

	def save_changelog(self, *args):
		if self.changelog_file:
			self.changelog_lock.acquire()
			handle = open(self.changelog_file, "a")
			handle.write(datetime.datetime.now().strftime("%d%m%Y %H:%M:%S.%f\t"))
			handle.write('\t'.join(map(str, args)))
			handle.write('\n')
			handle.close()
			self.changelog_lock.release()

	def load_headers(self, file):
		if file.startswith("http://"):
			self.script = "<script src=\"%s\" type=\"text/javascript\"></script>\n" % file
		else:
			self.script = open(file).read()

	def load_global(self, file):
		if not os.path.exists(file):
			raise IOError("File '%s' not found." % file)
		self.cpp_tagger.load_global(file)

	def load_local(self, file):
		if not os.path.exists(file):
			raise IOError("File '%s' not found." % file)
		self.cpp_tagger.load_local(file)

	def load_names(self, file1, file2):
		if not os.path.exists(file1):
			raise IOError("File '%s' not found." % file1)
		if not os.path.exists(file2):
			raise IOError("File '%s' not found." % file2)
		self.cpp_tagger.load_names(file1, file2)

	def set_styles(self, styles, types={}):
		self.styles = styles
		self.types = types

	def postprocess_document(self, uri, document):
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

	def add_name(self, name, entity_type, entity_identifier, document_id=None):
		if not self.check_name(name, entity_type, entity_identifier):
			self.cpp_tagger.add_name(name, int(entity_type), entity_identifier)
			self.save_changelog("AddName", name, entity_type, entity_identifier)
		if document_id:
			self.document_types_lock.acquire()
			if document_id not in self.document_types:
				self.document_types[document_id] = set()
			self.document_types[document_id].add(int(entity_type))
			self.document_types_lock.release()
			self.allow_name(name, document_id)

	def allow_name(self, name, document_id):
		if self.is_blocked(name, document_id):
			self.cpp_tagger.allow_block_name(name, document_id, False)
			self.save_changelog("AllowName", name, document_id)

	def block_name(self, name, document_id):
		if not self.is_blocked(name, document_id):
			self.cpp_tagger.allow_block_name(name, document_id, True)
			self.blocked_documents_lock.acquire()
			if name not in self.blocked_documents:
				self.blocked_documents[name] = set()
			self.blocked_documents[name].add(document_id)
			if len(self.blocked_documents[name]) == 5:
				self.cpp_tagger.allow_block_name(name, None, True)
			self.blocked_documents_lock.release()
			self.save_changelog("BlockName", name, document_id)

	def check_name(self, name, entity_type, entity_identifier):
		return self.cpp_tagger.check_name(name, int(entity_type), entity_identifier)

	def is_blocked(self, name, document_id):
		return self.cpp_tagger.is_blocked(document_id, name)

	def get_matches(self, document, document_id, entity_types, auto_detect=True, allow_overlap=False, protect_tags=True, max_tokens=5, tokenize_characters=False, ignore_blacklist=False, utf8_coordinates=False):
		if not PY3:
			if isinstance(document, unicode):
				document = document.encode("utf8")
				utf8_coordinates = True
		document_id = str(document_id)
		entity_types = set(entity_types)
		self.document_types_lock.acquire()
		if document_id in self.document_types:
			entity_types.update(self.document_types[document_id])
		self.document_types_lock.release()
		params = tagger_swig.GetMatchesParams()
		for entity_type in entity_types:
			params.add_entity_type(entity_type)
		params.auto_detect = auto_detect
		params.allow_overlap = allow_overlap
		params.protect_tags = protect_tags
		params.max_tokens = max_tokens
		params.tokenize_characters = tokenize_characters
		params.ignore_blacklist = ignore_blacklist
		matches = self.cpp_tagger.get_matches(document, document_id, params)
		if utf8_coordinates:
			mapping = {}
			byte = 0
			char = 0
			u_document = document.decode("utf8")
			for b in u_document:
				u = b.encode("utf8")
				char_bytes = len(u)
				for i in range(0, char_bytes):
					mapping[byte+i] = char
				byte += char_bytes
				char += 1
			u_matches = []
			for match in matches:
				u_matches.append((mapping[match[0]], mapping[match[1]], match[2]))
			return u_matches
		else:
			return matches

	def get_entities(self, document, document_id, entity_types, auto_detect=True, allow_overlap=False, protect_tags=True, max_tokens=5, tokenize_characters=False, ignore_blacklist=False, format='xml'):
		if format is None:
			format = "xml"
		format = format.lower()
		matches = self.get_matches(document, document_id, entity_types, auto_detect, allow_overlap, protect_tags, max_tokens, tokenize_characters, ignore_blacklist)
		doc = []
		if format == "xml":
			uniq = {}
			for match in matches:
				if match[2] is not None:
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
				for entity_type, entity_identifier in match[2]:
					doc.append('<entity>')
					doc.append('<type xsi:type="xsd:int">%i</type>' % entity_type)
					doc.append('<identifier xsi:type="xsd:string">%s</identifier>' % entity_identifier)
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
				if match[2] is not None:
					for entity_type, entity_identifier in match[2]:
						key = (text, entity_type, str(entity_identifier))
						if key not in uniq:
							uniq[key] = 1
			sep = '\t'
			if format == 'tsv':
				sep = '\t'
			elif format == 'csv':
				sep = ','
			elif format == 'ssv':
				sep = ';'
			for text, entity_type, entity_identifier in uniq:
				if sep in text:
					text = '"' + text + '"'
				if sep in entity_identifier:
					entity_identifier = '"' + entity_identifier + '"'
				doc.append('%s%s%i%s%s' % (text, sep, entity_type, sep, entity_identifier))
			doc = '\n'.join(doc)
		return doc

	def create_html(self, document, document_id, matches, basename="tagger", add_events=False, extra_classes=False, force_important=False, html_footer=""):
		matches.sort()
		doc = []
		i = 0
		all_types = set()
		divs = {}
		priority_type_rule = {}
		for priority in self.types:
			priority_type_rule[priority] = lambda x: eval(self.types[priority])
		for match in matches:
			length = match[0]-i
			if length > 0:
				doc.append(document[i:match[0]])
			if match[2] is not None:
				text = document[match[0]:match[1]+1]
				str = []
				match_classes = [basename+"_match"]
				match_types = set()
				for entity_type, entity_identifier in match[2]:
					str.append("%i.%s" % (entity_type, entity_identifier))
					all_types.add(entity_type)
					match_types.add(entity_type)
					if extra_classes:
						match_classes.append(entity_identifier)
				match_style = ''
				for priority in sorted(self.styles.iterkeys()):
					if priority not in self.types:
						match_style = self.styles[priority]
						break
					for entity_type in match_types:
						if priority_type_rule[priority](entity_type):
							match_style = self.styles[priority]
							break
					if match_style:
						break
				if force_important:
					match_style = " !important;".join(match_style.split(";"))
				md5 = hashlib.md5()
				str = ";".join(str)
				md5.update(str)
				key = md5.hexdigest()
				divs[key] = str
				doc.append('<span class="%s" style="%s" ' % (' '.join(match_classes), match_style))
				if add_events:
					doc.append('onMouseOver="startReflectPopupTimer(\'%s\',\'%s\')" ' % (key, text))
					doc.append('onMouseOut="stopReflectPopupTimer()" ')
					doc.append('onclick="showReflectPopup(\'%s\',\'%s\')"' % (key, text))
				doc.append('>')
				doc.append(text)
				doc.append('</span>')
			i = match[1] + 1
		if i < len(document):
			length = len(document)-i
			doc.append(document[i:i+length+1])
		doc.append('<div class="%s_entities" style="display: none;">\n' % basename)
		for key in divs:
			str = divs[key]
			doc.append('  <span name="%s">%s</span>\n' % (key, str))
		doc.append('</div>\n')
		doc.append('<div style="display: none;" class="%s_entity_types">\n' % basename)
		for entity_type in all_types:
			doc.append('  <span>%i</span>\n' % entity_type)
		doc.append('</div>\n')
		doc.append('<div style="display: none;" id="%s_div_doi">%s</div>\n' % (basename, document_id))
		doc.append('<div name="reflect_v2" style="display: none;"></div>\n')
		doc.append(html_footer)
		doc.append('</body>\n</html>\n')
		return self.postprocess_document(document_id, ''.join(doc))

	def get_html(self, document, document_id, entity_types, auto_detect=True, allow_overlap=False, protect_tags=True, max_tokens=5, tokenize_characters=False, ignore_blacklist=False, basename='tagger', add_events=False, extra_classes=False, force_important=False, html_footer=''):
		matches = self.get_matches(document, document_id, entity_types, auto_detect, allow_overlap, protect_tags, max_tokens, tokenize_characters, ignore_blacklist)
		return self.create_html(document, document_id, matches, basename=basename, add_events=add_events, extra_classes=extra_classes, force_important=force_important, html_footer=html_footer)

	def get_jsonld(self, document, document_charset, document_id, annotation_index, entity_types, auto_detect=True, allow_overlap=False, protect_tags=True, max_tokens=5, tokenize_characters=False, ignore_blacklist=False):
		matches = self.get_matches(document, document_id, entity_types, auto_detect, allow_overlap, protect_tags, max_tokens, tokenize_characters, ignore_blacklist)
		base = "_:"
		if document_id is not None:
			base = document_id
		offsets = {}
		byte_offset = 0
		if not PY3:
			document = document.decode(document_charset)
		for char_offset, char in enumerate(document):
					offsets[byte_offset] = char_offset
					byte_offset += len(char.encode(document_charset))
		data = {}
		data["@context"] = ["http://nlplab.org/ns/restoa-context-20150307.json",  "http://nlplab.org/ns/bio-20151118.jsonld"]
		if annotation_index is None:
			data["@id"] = "_:annotations"
			data["@graph"] = []
			i = 0
			for match in matches:
				if match[0] in offsets and match[1] in offsets and match[2] is not None:
					annotation = {}
					annotation["@id"] = "_:annotations/%d" % i
					annotation["target"] = "%s#char=%d,%d" % (base, offsets[match[0]], offsets[match[1]]+1)
					if len(match[2]) == 1:
						annotation["body"] = entity_dict(match[2][0][0], match[2][0][1])
					else:
						annotation["body"] = [entity_dict(entity_type, entity_identifier) for entity_type, entity_identifier in match[2]]
					data["@graph"].append(annotation)
					i += 1
		else:
			data["@id"] = "_:annotations/%d" % annotation_index
			i = 0
			for match in matches:
				if match[0] in offsets and match[1] in offsets and match[2] is not None:
					if i == int(annotation_index):
						data["target"] = "%s#char=%d,%d" % (base, offsets[match[0]], offsets[match[1]]+1)
						if len(match[2]) == 1:
							data["body"] = entity_dict(match[2][0][0], match[2][0][1])
						else:
							data["body"] = [entity_dict(entity_type, entity_identifier) for entity_type, entity_identifier in match[2]]
						break
					i += 1
		return json.dumps(data, separators=(',', ':'), sort_keys=True)

	def resolve_name(self, name):
		return self.cpp_tagger.resolve_name(name)
