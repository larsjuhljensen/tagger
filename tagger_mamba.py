import os
import re
import sys
import hashlib

import mamba.http
import mamba.setup
import mamba.task
import mamba.util

try:
	import blackmamba.database
	import blackmamba.html
	import blackmamba.xpage
except ImportError:
	blackmamba = None

import tagger.tagger


class Setup(mamba.setup.Configuration):

	def __init__(self, ini_file):
		mamba.setup.Configuration.__init__(self, ini_file)
		sys.setcheckinterval(10000)
		styles = {}
		if "STYLES" in self.sections:
			for priority in self.sections["STYLES"]:
				styles[int(priority)] = self.sections["STYLES"][priority]
		types = {}
		if "TYPES" in self.sections:
			for priority in self.sections["TYPES"]:
				types[int(priority)] = self.sections["TYPES"][priority]
		print '[INIT]  Loading tagger ...'
		self.tagger = tagger.tagger.Tagger()
		self.tagger.set_styles(styles, types)
		if "java_scripts" in self.globals:
			self.tagger.load_headers(self.globals["java_scripts"])
		self.tagger.load_names(self.globals["entities_file"], self.globals["names_file"])
		if "global_file" in self.globals:
			self.tagger.load_global(self.globals["global_file"])
		if "local_file" in self.globals:
			self.tagger.load_local(self.globals["local_file"])
		if "changelog_file" in self.globals:
			self.tagger.load_changelog(self.globals["changelog_file"])


class AddName(mamba.task.Request):

	def main(self):
		rest = mamba.task.RestDecoder(self)
		for check in ("name", "document_id", "entity_type", "entity_identifier"):
			if check not in rest:
				raise mamba.task.SyntaxError, 'Required parameter "%s" missing.' % check
		mamba.setup.config().tagger.add_name(mamba.util.string_to_bytes(rest["name"], "utf-8"), int(rest["entity_type"]), mamba.util.string_to_bytes(rest["entity_identifier"], "utf-8"), mamba.util.string_to_bytes(rest["document_id"], "utf-8"))
		mamba.http.HTTPResponse(self, "AddName succeeded.").send()


class AllowName(mamba.task.Request):

	def main(self):
		rest = mamba.task.RestDecoder(self)
		for check in ("name", "document_id"):
			if check not in rest:
				raise mamba.task.SyntaxError, 'Required parameter "%s" missing.' % check
		mamba.setup.config().tagger.allow_name(mamba.util.string_to_bytes(rest["name"], "utf-8"), mamba.util.string_to_bytes(rest["document_id"], "utf-8"))
		mamba.http.HTTPResponse(self, 'AllowName succeeded.').send()


class BlockName(mamba.task.Request):

	def main(self):
		rest = mamba.task.RestDecoder(self)
		for check in ("name", "document_id"):
			if check not in rest:
				raise mamba.task.SyntaxError, 'Required parameter "%s" missing.' % check
		mamba.setup.config().tagger.block_name(mamba.util.string_to_bytes(rest["name"], "utf-8"), mamba.util.string_to_bytes(rest["document_id"], "utf-8"))
		mamba.http.HTTPResponse(self, 'BlockName succeeded.').send()


class GetHead(mamba.task.EmptyRequest):

	def main(self):
		mamba.http.HTTPResponse(self, mamba.setup.config().globals["java_scripts"]).send()


class GetHeader(GetHead):
	pass


class GetPopup(mamba.task.Request):

	def main(self):
		rest = mamba.task.RestDecoder(self)
		entities = mamba.setup.config().tagger.resolve_name(mamba.util.string_to_bytes(rest["name"], self.http.charset))
		if len(entities):
			url_params = []
			show_first = ["9606", "10090"]
			for pref_organism in show_first:
				for type, id in entities:
					if type == pref_organism:
						url_params.append(type + "." + id + "+")
			for type, id in entities:
				if type not in show_first:
					url_params.append(type + "." + id + "+")
			popup_url = 'http://reflect.ws/popup/fcgi-bin/createPopup.fcgi?' + ''.join(url_params)
			popup_url = popup_url[:-1]
			reply = mamba.http.HTTPRedirect(self, popup_url)
			reply.send()
		else:
			html = "<html><head><title>No Reflect popup available</title></head><body>The name '%s' was not found in our dictionary.</body></html>" % self.names[0]
			mamba.http.HTTPResponse(self, html, content_type="text/html").send()


class ResolveName(mamba.task.Request):

	def main(self):
		rest = mamba.task.RestDecoder(self)
		names = []
		if "name" in rest:
			names.append(mamba.util.string_to_bytes(rest["name"], self.http.charset))
		elif "names" in rest:
			names += mamba.util.string_to_bytes(rest["names"], self.http.charset).split("\n")
		else:
			raise mamba.task.SyntaxError, 'Required parameter "name" or "names" is missing.'
		format = "tsv"
		if "format" in rest:
			format = rest["format"]
		if format == "xml":
			xml = []
			xml.append("""<?xml version="1.0" encoding="UTF-8"?>\n""")
			xml.append("""<ResolveNameResponse xmlns="Reflect" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">""")
			xml.append("""<items>""")
			for name in names:
				xml.append("""<item><name xsi:type="xsd:string">%s</name><entities>""" % name)
				for entity in mamba.setup.config().tagger.resolve_name(name):
					xml.append("""<entity><type xsi:type="xsd:int">%d</type><identifier xsi:type="xsd:string">%s</identifier></entity>""" % (entity[0], entity[1]))
				xml.append("""</entities></item>""")
			xml.append("</items>")
			xml.append("</ResolveNameResponse>")
			result = "".join(xml)
			content_type = "text/xml"
		else:
			tsv = []
			for name in names:
				for entity in mamba.setup.config().tagger.resolve_name(name):
					tsv.append("%s\t%d\t%s\n" % (name, entity[0], entity[1]))
			result = "".join(tsv)
			content_type = "text/plain"
		mamba.http.HTTPResponse(self, result, content_type).send()


_taggable_types = ["text/html", "text/plain", "text/xml", "text/tab-separated-values", 'application/msword', 'application/pdf', 'application/vnd.ms-excel']


class TaggingRequest(mamba.task.Request):

	def __init__(self, http, action):
		mamba.task.Request.__init__(self, http, action, priority=1)

	def parse(self, rest):
		self.hash = None
		if "hash" in rest:
			self.hash = rest["hash"]
		self.document = None
		if "document" in rest:
			self.document = rest["document"]
		elif "text" in rest:
			self.document = rest["text"]
		if self.document != None and "content_type" in rest and rest["content_type"].lower() == "text/html" or isinstance(self.document, str):
			self.document = unicode(self.document, self.http.charset, errors="replace")
		self.document_id = None
		self.document_url = None
		doi = None
		if "doi" in rest:
			doi  = rest["doi"]
		elif "document_identifier" in rest:
			doi = rest["document_identifier"]
		if doi:
			if self.doi.startswith('doi:'):
				doi = doi[4:]
			self.document_id = doi
			self.document_url = "http://dx.doi.org/"+doi
		elif "uri" in rest:
			if rest["uri"].startswith("http"):
				self.document_id = rest["uri"]
				self.document_url = rest["uri"]
			else:
				self.document_id = "http://"+rest["uri"]
				self.document_url = "http://"+rest["uri"]
		elif "url" in rest:
			if rest["url"].startswith("http"):
				self.document_id = rest["url"]
				self.document_url = rest["url"]
			else:
				self.document_id = "http://"+rest["url"]
				self.document_url = "http://"+rest["url"]
		elif "pmid" in rest:
			self.document_id = rest["pmid"]
			self.document_url = "http://www.ncbi.nlm.nih.gov/pubmed/"+rest["pmid"]
		elif self.hash != None:
			self.document_id = self.hash

		self.auto_detect = 1
		if "auto_detect" in rest:
			self.auto_detect = int(rest["auto_detect"])
		self.auto_detect_doi = 1
		if "auto_detect_doi" in rest:
			self.auto_detect_doi = int(rest["auto_detect_doi"])
		self.ignore_blacklist = 0
		if "ignore_blacklist" in rest:
			self.ignore_blacklist = int(rest["ignore_blacklist"])

		if "entity_types" in rest:
			self.entity_types = set()
			for type in rest["entity_types"].split():
				self.entity_types.add(int(type))
		else:
			self.entity_types = set()
			if hasattr(self, "user_settings"):
				if "proteins" in self.user_settings and mamba.setup.config_is_true(self.user_settings["proteins"]):
					self.entity_types.add(9606)
				if "chemicals" in self.user_settings and mamba.setup.config_is_true(self.user_settings["chemicals"]):
					self.entity_types.add(-1)
				if "wikipedia" in self.user_settings and mamba.setup.config_is_true(self.user_settings["wikipedia"]):
					self.entity_types.add(-11)

	def convert(self):
		md5 = hashlib.md5()
		md5.update(self.document)
		key = md5.hexdigest()
		if 'bin_dir' in mamba.setup.config().globals:
			bin_dir = mamba.setup.config().globals["bin_dir"]
		else:
			bin_dir = "./bin"
		infile  = "/dev/shm/"+key
		outfile = "/dev/shm/"+key+".html"
		f = open(infile, 'w')
		f.write(self.document)
		f.flush()
		f.close()
		if self.document.startswith('%PDF'):
			os.system("%s/pdf2html %s >& /dev/null" % (bin_dir, infile))
		elif os.system("unset DISPLAY; abiword --to=html --exp-props='embed-css: yes; embed-images: yes;' %s" % infile):
			os.system("%s/xls2csv %s | %s/csv2html > %s" % (bin_dir, infile, bin_dir, outfile))
		f = open(outfile, "r")
		html = f.read();
		f.close()
		os.remove(infile)
		os.remove(outfile)
		if len(html):
			self.document = unicode(html, "utf-8", "replace")
			self.queue("main")
		else:
			mamba.http.HTTPErrorResponse(self, 400, "Request contains an unsupported document type").send()

	def download(self):
		self.info("Downloading: %s" % self.document_url)
		page, status, headers, page_url, charset = mamba.http.Internet().download(self.document_url)
		if charset:
			page = unicode(page, charset, "replace")
			self.http.charset = charset
		if status != 200:
			mamba.http.HTTPErrorResponse(self, status, page).send()
		else:
			page_is_text = False
			if "Content-Type" in headers:
				for accepted in _taggable_types:
					if headers["Content-Type"].lower().startswith(accepted):
						page_is_text = True
						break
			if not page_is_text:
				mamba.http.Redirect(self, location=page_url).send()
			elif page:
				self.uri = page_url # URI could have been changed via multiple redirects.
				self.document = page
				self.queue("main")
			else:
				mamba.http.HTTPErrorResponse(self, 404, "Unable to download URI: '%s'" % str(fetch_url)).send()

	def main(self):
		if not hasattr(self, "document"):
			self.parse(mamba.task.RestDecoder(self))
		if self.http.method == "OPTIONS":
			mamba.http.HTTPResponse(self, "").send()
		elif isinstance(self.document, unicode):
			self.queue("tagging")
		elif self.document != None:
			self.queue("convert")
		elif self.document_url != None:
			self.queue("download")
		elif self.hash != None:
			self.load()
		else:
			mamba.http.HTTPErrorResponse(self, 400, "Request is missing a document and has no uri, doi or pmid either.").send()


class OpenAnnotation(TaggingRequest):

	def __init__(self, http, action = "OpenAnnotation"):
		TaggingRequest.__init__(self, http, action)

	def parse(self, rest):
		TaggingRequest.parse(self, rest)
		self.annotation_index = None
		if "annotation_index" in rest:
			self.annotation_index = int(rest["annotation_index"])
		if not "entity_types" in rest:
			self.entity_types = set([9606,-1,-2,-22,-25,-26,-27])

	def tagging(self):
		data = mamba.setup.config().tagger.get_jsonld(document=mamba.util.string_to_bytes(self.document, self.http.charset), document_charset=self.http.charset, document_id=self.document_id, annotation_index=self.annotation_index, entity_types=self.entity_types, auto_detect=self.auto_detect, ignore_blacklist=self.ignore_blacklist)
		mamba.http.HTTPResponse(self, data, "application/ld+json").send()


class GetEntities(TaggingRequest):

	def __init__(self, http, action = "GetEntities"):
		TaggingRequest.__init__(self, http, action)

	def parse(self, rest):
		TaggingRequest.parse(self, rest)
		self.format = "xml"
		if "format" in rest:
			self.format = rest["format"].lower()
		if self.format not in ("xml", "tsv", "csv", "ssv"):
			raise mamba.task.SyntaxError, 'In action: %s unknown format: "%s". Supports only: %s' % (self.action, format, ', '.join(supported_formats))

	def tagging(self):
		data = mamba.setup.config().tagger.get_entities(document=mamba.util.string_to_bytes(self.document, self.http.charset), document_id=self.document_id, entity_types=self.entity_types, auto_detect=self.auto_detect, ignore_blacklist=self.ignore_blacklist, format=self.format)
		if format == "xml":
			mamba.http.HTTPResponse(self, data, "text/xml").send()
		else:
			mamba.http.HTTPResponse(self, data, "text/plain").send()


class GetHTML(TaggingRequest):

	def __init__(self, http):
		TaggingRequest.__init__(self, http, "GetHTML")

	def load(self):
		try:
			f = open(self.worker.params["tmp_dir"]+"/"+self.hash+".html", "r")
			self.document = f.read()
			f.close()
			mamba.http.HTMLResponse(self, self.document).send()
		except:
			mamba.http.HTTPErrorResponse(self, 404, "Pretagged document not available.").send()

	def respond(self):
		if self.hash != None:
			self.save()
		mamba.http.HTMLResponse(self, self.document).send()

	def save(self):
		if self.hash == None:
			hash  = hashlib.md5()
			hash.update(mamba.util.string_to_bytes(self.document, self.http.charset))
			self.hash = hash.hexdigest()
		f = open(self.worker.params["tmp_dir"]+"/"+self.hash+".html", "w")
		f.write(mamba.util.string_to_bytes(self.document, self.http.charset))
		f.close()

	def tagging(self):
		footer = ['<div class="reflect_user_settings" style="display: none;">']
		for key in self.user_settings:
			footer.append('  <span name="%s">%s</span>' % (key, self.user_settings[key]))
		footer.append('</div>\n')
		self.document = mamba.setup.config().tagger.get_html(document=mamba.util.string_to_bytes(self.document, self.http.charset), document_id=self.document_id, entity_types=self.entity_types, auto_detect=self.auto_detect, ignore_blacklist=self.ignore_blacklist, basename='reflect', add_events=True, extra_classes=False, force_important=True, html_footer="\n".join(footer))
		self.respond()


class GetURI(GetHTML):

	def __init__(self, http):
		TaggingRequest.__init__(self, http, "GetURI")

	def respond(self):
		self.save()
		mamba.http.HTTPResponse(self, self.worker.params["tmp_dir"]+"/"+self.hash+".html", "text/plain").send()


class Extract(TaggingRequest):
	
	def __init__(self, http, action = "Extract"):
		TaggingRequest.__init__(self, http, action)
	
	def tagging(self):
		dictionary = blackmamba.database.Connect("dictionary")
		document = mamba.util.string_to_bytes(self.document, self.http.charset)
		tagger = mamba.setup.config().tagger
		matches = tagger.get_matches(document=document, document_id=self.document_id, entity_types=self.entity_types, auto_detect=self.auto_detect, ignore_blacklist=self.ignore_blacklist)
		rows = {}
		for match in reversed(matches):
			classes = ["extract_match"]
			entities = match[2]
			if entities != None:
				for entity in entities:
					classes.append(entity[1])
					row = (blackmamba.database.preferred_type_name(entity[0], dictionary), blackmamba.html.xcase(blackmamba.database.preferred_name(entity[0], entity[1], dictionary)), entity[0], entity[1])
					if not row in rows:
						rows[row] = set()
					rows[row].add(document[match[0]:match[1]+1])
		document = tagger.create_html(document=document, document_id=None, matches=matches, basename='extract', add_events=False, extra_classes=True, force_important=False, html_footer="")
		page = blackmamba.xpage.XPage(self.action)
		selection = blackmamba.html.XDiv(page.content, "ajax_table")
		blackmamba.html.XH2(selection, "table_title").text = "Selected text"
		blackmamba.html.XP(selection).text = document
		tsv = []
		table = blackmamba.html.XDiv(page.content, "ajax_table")
		blackmamba.html.XH2(table, "table_title").text = "Identified terms"
		if len(rows):
			xtable = blackmamba.html.XDataTable(table)
			xtable["width"] = "100%"
			xtable.addhead("Type", "Name", "Identifier")
			for row in sorted(rows):
				tsv.append("%s\t%s\t%s\t%s\t%s\t%s\n" % (row[0], row[1], row[3], ";".join(sorted(rows[row])), self.document_url or "", self.document))
				url = blackmamba.database.url(row[2], row[3])
				if url:
					xtable.addrow(row[0], row[1], blackmamba.html.XLink(None, url, row[3], '_blank'))
				else:
					xtable.addrow(row[0], row[1], row[3])
			form = blackmamba.html.XForm(blackmamba.html.XP(table))
			blackmamba.html.XTextArea(form, attr = {"class" : "hidden", "id" : "clipboard"}).text = "".join(tsv)
			blackmamba.html.XLink(form, "", "Copy to clipboard", attr = {"class" : "button_link", "id" : "extract_copy_to_clipboard", "onClick" : "extract_copy_to_clipboard('clipboard'); return false;"})
			blackmamba.html.XLink(form, "", "Save to file", attr = {"class" : "button_link", "download" : "entities.tsv", "id" : "extract_save_to_file", "onClick" : "extract_save_to_file(this);"})
		else:
			blackmamba.html.XP(table).text = "No terms were identified in the selected text."
		mamba.http.HTMLResponse(self, page.tohtml()).send()


class ExtractPopup(Extract):

	def __init__(self, http, action = "ExtractPopup"):
		Extract.__init__(self, http, action)
