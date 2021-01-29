#!/usr/bin/env python3
# vim: ts=2 sw=2 et
#
# SPDX-FileCopyrightText: 2021 Sebastian Engel <kde@sebastianengel.eu>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
#
################################################################################
# The main purpose of this script is to create the Welcome.baskets from plain
# text.
# For a more convenient way for future edits, a simplified document model has
# been implemented here.
#

import re
import os
import shutil
import uuid

from enum import Enum
from datetime import datetime
from xml.dom import minidom




class Color:
  def __init__(self, color = ""):
    if color != "":
      if re.match('#[0-9ABCDEF]{6}',color.upper()) is None:
        raise TypeError('bgColor is not a proper RGB input, such as #000000')
    self.color = color

  def __str__(self):
    return str(self.color)

class NoteType(Enum):
  COLOR = "color"
  CROSSREFERENCE = "crossreference"
  FILE = "file"
  GROUP = "group"
  HTML = "html"
  IMAGE = "image"
  LAUNCHER = "launcher"
  LINK = "link"
  SOUND = "sound"
  TEXT = "text"

## NoteContent CrossRef
## TODO deal with variable directory names of target basket
class CrossRef:
  def __init__(self, title, url, icon = ""):
    if not isinstance(title, str):
      raise TypeError('title argument must be a string')
    self.title = title
    if not isinstance(icon, str):
      raise TypeError('icon argument must be a string')
    self.icon = icon
    if not isinstance(url, str):
      raise TypeError('url argument must be a string')
    self.url = url

## NoteContent Link
class LinkRef:
  def __init__(self, title, url, icon = "", autoTitle=True, autoIcon=True):
    if not isinstance(title, str):
      raise TypeError('title argument must be a string')
    self.title = title
    if not isinstance(icon, str):
      raise TypeError('icon argument must be a string')
    self.icon = icon
    if not isinstance(url, str):
      raise TypeError('url argument must be a string')
    self.url = url
    if not isinstance(autoTitle, bool):
      raise TypeError('autoTitle argument must be a boolean')
    self.autoTitle = autoTitle
    if not isinstance(autoIcon, bool):
      raise TypeError('autoIcon argument must be a boolean')
    self.autoIcon =autoIcon

## content argument is of variable type depending on notetype argument
class Note:
  def __init__(self, notetype, content, parent, tagStates = [], width = 0, x = 0, y = 0, 
      lastModification = datetime.now(), added = datetime.now(), folded=False):
    if not isinstance(notetype, NoteType):
      raise TypeError('notetype argument must be an NoteType enum')
    self.notetype = notetype

    if self.notetype == NoteType.COLOR:
      if not isinstance(content, Color):
        raise TypeError('The content argument must be of type Color for a COLOR note type')
    elif self.notetype == NoteType.CROSSREFERENCE:
      if not isinstance(content, CrossRef):
        raise TypeError('The content argument must be of type CrossRef for a CROSSREFERENCE note type')
    elif self.notetype == NoteType.FILE \
        or self.notetype == NoteType.SOUND \
        or self.notetype == NoteType.IMAGE \
        or self.notetype == NoteType.LAUNCHER:
      if not isinstance(content, str):
        raise TypeError('The content argument must be a string for a FILE, IMAGE, LAUNCHER, or SOUND note type')
      if not os.path.isfile(content):
        raise FileNotFoundError('The path in the content argument is not a file')
    elif self.notetype == NoteType.GROUP:
      for note in content:
        if not isinstance(note, Note):
          raise TypeError('The content argument must be a list of Note only for a GROUP note type')
    elif self.notetype == NoteType.HTML:
      if not isinstance(content, str):
        raise TypeError('The content for a HTML note type must be of type string')
    elif self.notetype == NoteType.LINK:
      if not isinstance(content, LinkRef):
        raise TypeError('The content for a Link note type must be of type LinkRef')
    elif self.notetype == NoteType.TEXT:
      if not isinstance(content, str):
        raise TypeError('The content for a TEXT note type must be of type string')
    self.content = content

    if not isinstance(parent, Note) and not isinstance(parent, Basket):
      raise TypeError('The parent argument must be of type Note, or of type Basket')
    self.parent = parent

    if self.notetype != NoteType.GROUP:
      for state in tagStates:
        if not isinstance(state, State):
          raise TypeError('There is an item in the tags list which is not of type Tag')
      self.tagStates = tagStates
    else:
      self.tagStates = []

    if not isinstance(width, int) and width >= 0:
      raise TypeError('The width argument must be an integer and greater/equal 0')
    self.width = width
    if not isinstance(x, int) and x >= 0:
      raise TypeError('The x argument must be an integer and greater/equal 0')
    self.x = x
    if not isinstance(y, int) and y >= 0:
      raise TypeError('The y argument must be an integer and greater/equal 0')
    self.y = y

    if not isinstance(lastModification, datetime):
      raise TypeError('the lastModification argument must be a datetime type')
    self.lastModification = lastModification
    if not isinstance(added, datetime):
      raise TypeError('the added argument must be a datetime type')
    self.added = added

    if self.notetype == NoteType.GROUP:
      if not isinstance(folded, bool):
        raise TypeError('the folded argument must be a boolean')
      self.folded = folded
    else:
      self.folded = False

  ## Fetch basket or try one level up
  def getBasket(self):
    if not isinstance(self.parent,Basket):
      return self.parent.getBasket()
    else:
      return self.parent

  ## returns a flat structure of self and notes
  def flatNoteFamily(self):
    flatlist = [self]

    if self.notetype == NoteType.GROUP:
      for child in self.content:
        flatlist.extend(child.flatNoteFamily())

    return flatlist



  def writeXmlElement(self, doc, parent, basketpath):

    if self.notetype == NoteType.GROUP:
      node = doc.createElement('group')

      ## column group does not have any attributes 
      if not isinstance(self.parent,Basket):
        if(self.parent.disposition != Disposition.FREE):
          for note in self.content:
            note.writeXmlElement(doc, node, basketpath)
      else:
        node.setAttribute('folded', 'true' if self.folded else 'false')

        if self.getBasket().disposition == Disposition.FREE:
          node.setAttribute('width', str(self.width))
          node.setAttribute('x', str(self.x))
          node.setAttribute('y', str(self.y))

        for note in self.content:
          note.writeXmlElement(doc, node, basketpath)

      parent.appendChild(node)
      
    # Non-Group notes
    else:
      node = doc.createElement('note')

      if isinstance(self.parent, Basket):
        node.setAttribute('width', str(self.width))
        node.setAttribute('x', str(self.x))
        node.setAttribute('y', str(self.y))
      elif self.parent.notetype != NoteType.GROUP:
        node.setAttribute('width', str(self.width))
        node.setAttribute('x', str(self.x))
        node.setAttribute('y', str(self.y))

      node.setAttribute('lastModification', self.lastModification.isoformat(timespec='seconds'))
      node.setAttribute('added', self.added.isoformat(timespec='seconds'))

      if self.notetype == NoteType.COLOR:
        node.setAttribute('type', 'color')

        content = doc.createElement('content')
        content.appendChild(doc.createTextNode(str(self.content)))
        node.appendChild(content)

      elif self.notetype == NoteType.CROSSREFERENCE:
        node.setAttribute('type', 'cross_reference')

        content = doc.createElement('content')
        content.setAttribute('title', str(self.content.title))
        content.setAttribute('icon', str(self.content.icon))

        content.appendChild(doc.createTextNode(str(self.content.url)))
        node.appendChild(content)

      elif self.notetype == NoteType.FILE:
        node.setAttribute('type', 'file')

        content = doc.createElement('content')
        content.appendChild(doc.createTextNode(os.path.basename(str(self.content))))
        node.appendChild(content)

      elif self.notetype == NoteType.HTML:
        node.setAttribute('type', 'html')

        ## TODO create file and copy in correct folder
        content = doc.createElement('content')
        
        filename = "item" + uuid.uuid1().hex + ".html"
        with open(basketpath + os.path.sep + filename, "w") as f:
          f.write(self.content)
        content.appendChild(doc.createTextNode(os.path.basename(filename)))
        node.appendChild(content)

      elif self.notetype == NoteType.IMAGE:
        node.setAttribute('type', 'image')

        content = doc.createElement('content')
        content.appendChild(doc.createTextNode(os.path.basename(str(self.content))))
        node.appendChild(content)

      elif self.notetype == NoteType.LAUNCHER:
        node.setAttribute('type', 'launcher')

        content = doc.createElement('content')
        content.appendChild(doc.createTextNode(os.path.basename(str(self.content))))
        node.appendChild(content)

      elif self.notetype == NoteType.LINK:
        node.setAttribute('type', 'link')

        content = doc.createElement('content')
        content.setAttribute('title', str(self.content.title))
        content.setAttribute('icon', str(self.content.icon))
        content.setAttribute('autoTitle', 'true' if self.content.autoTitle else 'false')
        content.setAttribute('autoIcon', 'true' if self.content.autoIcon else 'false')
        content.appendChild(doc.createTextNode(str(self.content)))
        node.appendChild(content)

      elif self.notetype == NoteType.SOUND:
        node.setAttribute('type', 'sound')

        content = doc.createElement('content')
        content.appendChild(doc.createTextNode(os.path.basename(str(self.content))))
        node.appendChild(content)

      elif self.notetype == NoteType.TEXT:
        node.setAttribute('type', 'text')

        ## TODO create file and copy in correct folder
        content = doc.createElement('content')
        content.appendChild(doc.createTextNode(str(self.content)))
        node.appendChild(content)

      if len(self.tagStates) > 0 and self.notetype != NoteType.GROUP:
        tagstates = doc.createElement('tags')

        tags_list = []
        for state in self.tagStates:
          tags_list.append(state.stateid)

        tagstates.appendChild(doc.createTextNode(';'.join(tags_list)))
        node.appendChild(tagstates)

      parent.appendChild(node)

## Basket color settings
class Appearance:
  def __init__(self, bgColor = Color(), bgImage = "", textColor = Color()):

    if not isinstance(bgColor, Color):
      raise TypeError('bgColor  must be of type Color')
    self.bgColor = bgColor

    self.bgImage = bgImage

    if not isinstance(textColor, Color):
      raise TypeError('textColor  must be of type Color')
    self.textColor = textColor


## Basket layout
class Disposition(Enum):
  FREE = "free"
  COL1 = "col1"
  COL2 = "col2"
  COL3 = "col3"


class Shortcut:
  def __init__(self, combination = ""):
    self.combination = combination


class Basket:
  def __init__(self, name, notes = [], suggestedFoldername="", children = [], icon = "knotes", 
      appearance = Appearance(), disposition = Disposition.FREE, shortcut = Shortcut(), folded = False):

    if not isinstance(name,str):
      raise TypeError('basket name is not of type string')
    self.name = name

    if not isinstance(suggestedFoldername,str):
      raise TypeError('basket suggestedFoldername is not of type string')
    self.suggestedFoldername = suggestedFoldername

    for basket in children:
      if not isinstance(basket, Basket):
        raise TypeError('There is a child basket which is not of type Basket')
    self.children = children


    for note in notes:
      if not isinstance(note, Note):
        raise TypeError('There is an item in a Basket which is not of type Note')
    self.notes = notes

    if not isinstance(icon, str):
      raise TypeError('basket icon is not of type string')
    self.icon = icon

    if not isinstance(appearance, Appearance):
      raise TypeError('appearance must be of type Appearance')
    self.appearance = appearance

    if not isinstance(disposition, Disposition):
      raise TypeError('disposition must be of type Disposition Enum')
    self.disposition = disposition

    if not isinstance(shortcut, Shortcut):
      raise TypeError('shortcut must be of type Shortcut')
    self.shortcut = shortcut

    if not isinstance(folded, bool):
      raise TypeError('folded switch must be of type bool')
    self.folded = folded

  def __str__(self):
    return self.name

  # returns self and all children in a list
  def flatBasketFamily(self):
    flatlist = [self]

    for child in self.children:
      flatlist.extend(child.flatBasketFamily())

    return flatlist

  def flatNoteFamily(self):
    flatlist = []

    for note in self.notes:
      flatlist.extend(note.flatNoteFamily())

    return flatlist

  def writeBasket(self, basketpath):

    imp = minidom.DOMImplementation()
    doctype = imp.createDocumentType(qualifiedName='basket', publicId='', systemId='')
    doc = imp.createDocument(None, 'basket', doctype)
    root = doc.documentElement

    properties = doc.createElement('properties')

    name = doc.createElement('name')
    name.appendChild(doc.createTextNode(str(self.name)))
    properties.appendChild(name)

    icon = doc.createElement('icon')
    icon.appendChild(doc.createTextNode(os.path.basename(str(self.icon))))
    properties.appendChild(icon)

    appearance = doc.createElement('appearance')
    appearance.setAttribute('backgroundColor', str(self.appearance.bgColor))
    appearance.setAttribute('backgroundImage', self.appearance.bgImage)
    appearance.setAttribute('textColor', str(self.appearance.textColor))
    properties.appendChild(appearance)

    disposition = doc.createElement('disposition')
    disposition.setAttribute('mindMap', 'false')
    if self.disposition == Disposition.FREE:
      disposition.setAttribute('columnCount', '0')
      disposition.setAttribute('free', 'true')
    else:
      if self.disposition == Disposition.COL1:
        disposition.setAttribute('columnCount','1')
      if self.disposition == Disposition.COL2:
        disposition.setAttribute('columnCount','2')
      if self.disposition == Disposition.COL3:
        disposition.setAttribute('columnCount','3')

      disposition.setAttribute('free', 'false')
    properties.appendChild(disposition)


    shortcut = doc.createElement('shortcut')
    shortcut.setAttribute('combination', self.shortcut.combination)
    shortcut.setAttribute('action', 'show')
    properties.appendChild(shortcut)


    key = doc.createElement('protection')
    key.setAttribute('key','')
    key.setAttribute('type','0')

    properties.appendChild(key)

    root.appendChild(properties)

    notes = doc.createElement('notes')

    for note in self.notes:
      note.writeXmlElement(doc,notes,basketpath)

    root.appendChild(notes)

    with open(basketpath + os.path.sep + ".basket", "w") as f:
      doc.writexml(f,indent="", addindent=" ", newl='\n', encoding="UTF-8")

  def createBasketTreeXmlElement(self, doc, parent):
      node = doc.createElement('basket')

      if len(self.children) != 0:
        node.setAttribute('folded', 'True' if self.folded  else 'False')

      node.setAttribute('folderName', os.path.basename(str(self.suggestedFoldername)) + os.path.sep)

      properties = doc.createElement('properties')

      name = doc.createElement('name')
      name.appendChild(doc.createTextNode(str(self.name)))
      properties.appendChild(name)

      icon = doc.createElement('icon')
      icon.appendChild(doc.createTextNode(os.path.basename(str(self.icon))))
      properties.appendChild(icon)

      appearance = doc.createElement('appearance')
      appearance.setAttribute('backgroundColor', str(self.appearance.bgColor))
      appearance.setAttribute('backgroundImage', self.appearance.bgImage)
      appearance.setAttribute('textColor', str(self.appearance.textColor))
      properties.appendChild(appearance)

      disposition = doc.createElement('disposition')
      disposition.setAttribute('mindMap', 'false')
      if self.disposition == Disposition.FREE:
        disposition.setAttribute('columnCount', '0')
        disposition.setAttribute('free', 'true')
      else:
        if self.disposition == Disposition.COL1:
          disposition.setAttribute('columnCount','1')
        if self.disposition == Disposition.COL2:
          disposition.setAttribute('columnCount','2')
        if self.disposition == Disposition.COL3:
          disposition.setAttribute('columnCount','3')

        disposition.setAttribute('free', 'false')
      properties.appendChild(disposition)


      shortcut = doc.createElement('shortcut')
      shortcut.setAttribute('combination', self.shortcut.combination)
      shortcut.setAttribute('action', 'show')
      properties.appendChild(shortcut)


      key = doc.createElement('protection')
      key.setAttribute('key','')
      key.setAttribute('type','0')

      properties.appendChild(key)

      node.appendChild(properties)

      for child in self.children:
        child.createBasketTreeXmlElement(doc,node)

      parent.appendChild(node)


class BasketTree:
  def __init__(self, baskets):
    for basket in baskets:
      if not isinstance(basket, Basket):
        raise TypeError('There is an item in a BasketTree which is not of type Basket: ' + str(basket))
    self.baskets = baskets

  ## writes the baskets.xml into path
  def writeTree(self, basketDirectory):
    if not isinstance(basketDirectory, BasketDirectory):
      raise TypeError('the argument basketDirectory is not of type BasketDirectory')

    ## test whether all basket have valid suggestedFoldernames, set unique if necessary
    foldernamesList = []
    for basket in self.flatBasketFamily():
      if basket.suggestedFoldername == "" or basket.suggestedFoldername in foldernamesList:
        suggestedFoldername = 'basket-' + uuid.uuid1().hex
      foldernamesList.append(basket.suggestedFoldername)

    ## create basketTree xml file
    imp = minidom.DOMImplementation()
    doctype = imp.createDocumentType(qualifiedName='basketTree', publicId='', systemId='')
    doc = imp.createDocument(None, 'basketTree', doctype)
    root = doc.documentElement

    for basket in self.baskets:
      basket.createBasketTreeXmlElement(doc,root)

    # marking the first basket as default
    if len(self.baskets) > 0:
      firstBasket = doc.getElementsByTagName('basket')[0]
      firstBasket.setAttribute('lastOpen', 'true')

    with open(basketDirectory.basketsPath + os.path.sep + "baskets.xml", "w") as f:
      doc.writexml(f,indent="", addindent=" ", newl='\n', encoding="UTF-8")

  def flatBasketFamily(self):
    flatlist = []
    for basket in self.baskets:
      flatlist.extend(basket.flatBasketFamily())
    return flatlist

## Tag's text style
class TextStyle:
  def __init__(self, strikeOut=False, bold = False, underline = False, color = Color(), italic = False):
    if not isinstance(strikeOut, bool):
      raise TypeError('strikeout must be a bool')
    self.strikeOut = strikeOut

    if not isinstance(bold, bool):
      raise TypeError('bold must be a boolean')
    self.bold = bold

    if not isinstance(underline, bool):
      raise TypeError('underline must be a boolean')
    self.underline = underline

    if not isinstance(color, Color):
      raise TypeError('color must be of type Color')
    self.color = color

    if not isinstance(italic, bool):
      raise TypeError('italic must be a boolean')
    self.italic = italic


## Tag's font
class Font:
  def __init__(self, name = "", size = -1):
    if not isinstance(name, str):
      raise TypeError('The Font name argument must be a string')
    self.name = name

    ## not sure whether this must be the case of wether a double is valid as well
    if not isinstance(size, int):
      raise TypeError('The Font\'s size argument must be of type integer')
    self.size = size

## Tag states
class State:
  def __init__(self, name, stateid, emblem = "", textstyle = TextStyle(), font = Font(),
      bgColor = Color(), textEquivalent = "", onAllTextLines = False):
    if not isinstance(name,str):
      raise TypeError('State name argument must be a string')
    self.name = name

    if not isinstance(stateid, str):
      raise TypeError('stateid argument must be a string')
    if re.match('[A-z0-9_]*', stateid) is None:
      raise TypeError('stateid argument must be a simplified ascii string [A-z0-9_]')
    self.stateid = stateid

    if not isinstance(emblem, str):
      raise TypeError('emblem argument must be a string')
    self.emblem = emblem

    if not isinstance(textstyle, TextStyle):
      raise TypeError('textstyle argument must be of type TextStyle')
    self.textstyle = textstyle

    if not isinstance(font, Font):
      raise TypeError('font argument must be of type Font')
    self.font = font

    if not isinstance(bgColor, Color):
      raise TypeError('bgColor argument must be of type Color')
    self.bgColor = bgColor

    if not isinstance(textEquivalent, str):
      raise TypeError('textEquivalent argument must be of type string')
    self.textEquivalent = textEquivalent

    if not isinstance(onAllTextLines, bool):
      raise TypeError('onAllTextLines argument must be a boolean')
    self.onAllTextLines = onAllTextLines

  def createXmlElement(self, doc, parent):
    node = doc.createElement('state')
    node.setAttribute('id', str(self.stateid))

    name = doc.createElement('name')
    name.appendChild(doc.createTextNode(str(self.name)))
    node.appendChild(name)

    emblem = doc.createElement('emblem')
    emblem.appendChild(doc.createTextNode(str(os.path.basename(self.emblem))))
    node.appendChild(emblem)

    textstyle = doc.createElement('text')
    textstyle.setAttribute('strikeOut', 'true' if self.textstyle.strikeOut else 'false')
    textstyle.setAttribute('bold', 'true' if self.textstyle.bold else 'false')
    textstyle.setAttribute('underline', 'true' if self.textstyle.underline else 'false')
    textstyle.setAttribute('color', str(self.textstyle.color))
    textstyle.setAttribute('italic', 'true' if self.textstyle.italic else 'false')
    node.appendChild(textstyle)

    font = doc.createElement('font')
    font.setAttribute('size', str(self.font.size))
    font.setAttribute('name', str(self.font.name))
    node.appendChild(font)


    bgcolor = doc.createElement('backgroundColor')
    bgcolor.appendChild(doc.createTextNode(str(self.bgColor)))
    node.appendChild(bgcolor)

    textequiv = doc.createElement('textEquivalent')
    textequiv.setAttribute('string', str(self.textEquivalent))
    textequiv.setAttribute('onAllTextLines', 'true' if self.onAllTextLines else 'false')
    node.appendChild(textequiv)

    parent.appendChild(node)


class Tag:
  def __init__(self, name, states = [], shortcut = Shortcut(), inherited=False):
    if not isinstance(name, str):
      raise TypeError('Tag name must be of type string')
    self.name = name

    for state in states:
      if not isinstance(state, State):
        raise TypeError('Every state in states of Tag must be of type State')
    self.states = states

    if not isinstance(shortcut, Shortcut):
      raise TypeError('The shortcut argument must be of type Shortcut')
    self.shortcut = shortcut

    if not isinstance(inherited, bool):
      raise TypeError('The inherited switch must be of type bool')
    self.inherited = inherited

  def createXmlElement(self, doc, parent):
    node = doc.createElement('tag')

    name = doc.createElement('name')
    name.appendChild(doc.createTextNode(str(self.name)))
    node.appendChild(name)

    shortcut = doc.createElement('shortcut')
    shortcut.appendChild(doc.createTextNode(str(self.shortcut.combination)))
    node.appendChild(shortcut)

    inherited = doc.createElement('inherited')
    inherited.appendChild(doc.createTextNode('true' if self.inherited else 'false'))
    node.appendChild(inherited)

    for state in self.states:
      state.createXmlElement(doc, node)

    parent.appendChild(node)

class BasketTagsTree:
  def __init__(self, tags):
    for tag in tags:
      if not isinstance(tag, Tag):
        raise TypeError('There is an item in BasketTagsTree which is not of type Tag')
    self.tags = tags

  # write tags.xml into path, and write emblems in respective path
  def writeTree(self, basketDirectory):
    if not isinstance(basketDirectory, BasketDirectory):
      raise TypeError('the argument basketDirectory is not of type BasketDirectory')

    imp = minidom.DOMImplementation()
    doctype = imp.createDocumentType(qualifiedName='basketTags', publicId='', systemId='')
    doc = imp.createDocument(None, 'basketTags', doctype)
    root = doc.documentElement

    ## TODO not sure whether this attribute is relevant, is present in original Welcome.baskets
    root.setAttribute('nextStateUid','6')

    for tag in self.tags:
      tag.createXmlElement(doc,root)

    with open(basketDirectory.savepath + os.path.sep + "tags.xml", "w") as f:
      doc.writexml(f,indent="", addindent=" ", newl='\n', encoding="UTF-8")
  



class BasketDirectory:
  def __init__(self, savepath,baskets = [], tags = [], additionalBackgrounds = [], additionalEmblems = [], additionalIcons = []):
    if not isinstance(savepath, str) and not os.path.isdir(savepath):
      raise FileNotFoundError('The savepath argument must be a valid directory')
    self.savepath = os.path.realpath(savepath)

    for basket in baskets:
      if not isinstance(basket, Basket):
        raise TypeError('the baskets argument must only contain objects of type Basket')
    self.baskets = baskets

    for tag in tags:
      if not isinstance(tag, Tag):
        raise TypeError('The tags argument must only contain objects of type Tag')
    self.tags = tags

    for bg in additionalBackgrounds:
      if not isinstance(bg, str) and not os.path.isfile(bg):
        raise FileNotFoundError('Every entry in additionalBackgrounds must be a valid file')
    self.additionalBackgrounds = additionalBackgrounds

    for emblem in additionalEmblems:
      if not isinstance(emblem, str) and not os.path.isfile(emblem):
        raise FileNotFoundError('Every entry in additionalEmblems must be a valid file')
    self.additionalEmblems = additionalEmblems

    for icon in additionalIcons:
      if not isinstance(icon, str) and not os.path.isfile(icon):
        raise FileNotFoundError('Every entry in additionalIcons must be a valid file')
    self.additionalIcons = additionalIcons

    # prepare directory structure
    self.basketsPath= self.savepath + os.path.sep + "baskets"
    self.tagsPath= self.savepath + os.path.sep + "tag-emblems"
    self.bgPath= self.savepath + os.path.sep + "backgrounds"
    self.iconsPath= self.savepath + os.path.sep + "basket-icons"

    self.basketTree = BasketTree(self.baskets)
    self.basketTagsTree = BasketTagsTree(self.tags)


  def write(self):

    if not os.path.isdir(self.savepath):
      os.makedirs(self.savepath, exist_ok=True)

    os.makedirs(self.basketsPath, exist_ok=True)
    os.makedirs(self.tagsPath, exist_ok=True)
    os.makedirs(self.bgPath, exist_ok=True)
    os.makedirs(self.iconsPath, exist_ok=True)

    ## copy the tag-emblems into the BasketDirectory
    for emblem in self.additionalEmblems:
      shutil.copyfile(emblem,self.tagsPath + os.path.sep + os.path.basename(emblem))

    for tag in self.tags:
      for state in tag.states:
        inDirPath = self.tagsPath + os.path.sep + os.path.basename(state.emblem)
        if not os.path.isfile(inDirPath) and state.emblem != "" \
          and os.path.isfile(state.emblem):
          # TODO figure out how to check if specified emblem is a valid icon:
          shutil.copyfile(os.path.realpath(state.emblem),self.tagsPath + os.path.sep + os.path.basename(state.emblem))

    ## add backgrounds to BasketDirectory
    #bgOrigins = self.additionalBackgrounds
    for bg in self.additionalBackgrounds:
      shutil.copyfile(os.path.realpath(bg),self.bgPath + os.path.sep + os.path.basename(bg))

    for basket in self.basketTree.flatBasketFamily():
      inDirPath = self.bgPath + os.path.sep + os.path.basename(basket.appearance.bgImage)
      if not os.path.isfile(inDirPath):
        shutil.copyfile(os.path.realpath(basket.appearance.bgImage), str(inDirPath))

      ## TODO deal with backgrounds having the same name but different origins
      # if not basket.appearance.bgImage in bgOrigins:
      #   if os.path.isfile(inDirPath):


    ## add icons to BasketDirectory
    for icon in self.additionalIcons:
      shutil.copyfile(os.path.realpath(icon),self.bgPath + os.path.sep + os.path.basename(icon))

    for basket in self.basketTree.flatBasketFamily():
      inDirPath = self.iconsPath + os.path.sep + os.path.basename(basket.icon)
      if not os.path.isfile(inDirPath) \
        and os.path.isfile(basket.icon):
        # TODO figure out how to check if specified emblem is a valid icon:
        shutil.copyfile(os.path.realpath(basket.icon), inDirPath)

    ## Write trees
    self.basketTree.writeTree(self)
    self.basketTagsTree.writeTree(self)

    ## Write individual baskets
    for basket in self.basketTree.flatBasketFamily():
      basketPath = self.basketsPath + os.path.sep + basket.suggestedFoldername

      os.makedirs(basketPath)

      # Prepare files into local folders
      for note in basket.flatNoteFamily():
        if note.notetype == NoteType.FILE \
              or note.notetype == NoteType.IMAGE \
              or note.notetype == NoteType.LAUNCHER \
              or note.notetype == NoteType.SOUND:
            shutil.copyfile(os.path.realpath(note.content), basketPath + os.path.sep + os.path.basename(str(note.content)))

      basket.writeBasket(basketPath)
