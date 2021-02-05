#!/usr/bin/env python3
# vim: ts=2 sw=2 et
#
# SPDX-FileCopyrightText: 2021 Sebastian Engel <kde@sebastianengel.eu>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
#
################################################################################

"""A simple module to create a BasKet directory source

This module is used to create a Basket directory source which subsequentially
can be compiled to a .baskets Basket Archive. Pybasket has a simplified document
model implement to give a somewhat convenient way to script the creation of
Baskets. Initially this module is used to create the examples and user documents
coming with BasKet Note Pads, the so called Welcome baskets.

The module is currently not designed to also parse basket sources.

  Typical usage example:

    from pybasket import *

    mybasket = Basket(name = "New Notes")

    note1 = Note(notetype = NoteType.HTML, parent = mybasket, width = 300, x = 0, y= 0,
        content ='''
    <html><head/><body>
    This is a note
    </body></html>
    ''')
    mybasket.notes.append(note1)

    mysubBasket = Basket(name = "Old Notes")

    note2 = Note(notetype = NoteType.HTML, parent = mysubBasket, width = 200, x = 0, y = 100,
        content = '''
    <html><head/><body>
    My old note
    </body></html>
    ''')
    mysubBasket.notes.append(note2)


    mybasket.children.append(mysubBasket)

    baskets = [ mybasket ]
    tags = []

    ## Compile directory and save
    basketDir = BasketDirectory("my_source", baskets, tags)

    basketDir.write()

"""



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
  """A simple class to hold a color value string.

    Example usage:
      foo = Color(color='#ffffff')

  Attributes:
      color:  a color value in the format of rgb hex triplet, '#rrggbb', or 
        empty string to specifiy to use the application's default color
  """

  def __init__(self, color: str = ""):
    if not isinstance(color, str):
      raise TypeError('the color argument must be of type string, but got %s' % type(color))

    if color != "" and re.match('#[0-9ABCDEF]{6}', color.upper()) is None:
      raise ValueError('color is not a proper RGB input, such as #000000, or empty string, but got \'' + color + '\'')

    self.color = color

  def __str__(self):
    return str(self.color)

  def isDefault(self) -> bool:
    """Returns true if this object specifies the default application's color"""
    if self.color == "":
      return True
    else:
      return False

class NoteType(Enum):
  """An enum class defining the type of the Note class

  """

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
class CrossRef:
  """Container class which holds a cross link to another Basket

    Example Usage:
      CrossRef(title="My other basket", icon = "", url="basket://basket48")


  Todo:
      The CrossRef class is really useful as it is, since the url has to be
      selected by the user. However, the CrossRef class should determine the
      url itself from the referenced basket.

  Attributes:
      title: Label which is used instead of the url
      url: Relative path to another basket within the baskets directory,
        such as basket://basket49
      icon: Optional; the name of the icon, or the path to the icon file
  """

  def __init__(self, title: str, url: str , icon: str= ""):
    if not isinstance(title, str):
      raise TypeError('title argument must be a string, but got ' + type(title))
    self.title = title
    if not isinstance(icon, str):
      raise TypeError('icon argument must be a string, but got ' + type(icon))
    self.icon = icon
    if not isinstance(url, str):
      raise TypeError('url argument must be a string, but got ' + type(url))
    self.url = url

## NoteContent Link
class LinkRef:
  """A container class for web links

    Example usage:
      bar = LinkRef(title="my web link", url="https://invent.kde.org")

  Attributes:
      title: Label which is used instead of url
      url: Link to the external resource, such as https://kde.org
      icon: Optional; the name of the icon, or the path to the icon file
      autoTitle: Optional; if autoTitle is True, BasKet can decide to show a
        different title depending on context
      autoIcon: Optional; if autoIcon is True, BasKet can decide to show a 
        different icon, e.g. if the specified icon is not known
  """

  def __init__(self, title: str, url: str, icon:str = "", autoTitle:bool = True, autoIcon:bool = True):
    if not isinstance(title, str):
      raise TypeError('title argument must be a string, but got ' + type(title))
    self.title = title
    if not isinstance(icon, str):
      raise TypeError('icon argument must be a string, but got ' + type(icon))
    self.icon = icon
    if not isinstance(url, str):
      raise TypeError('url argument must be a string, but got ' + type(url))
    self.url = url
    if not isinstance(autoTitle, bool):
      raise TypeError('autoTitle argument must be a boolean, but got ' + type(autoTitle))
    self.autoTitle = autoTitle
    if not isinstance(autoIcon, bool):
      raise TypeError('autoIcon argument must be a boolean, but got ' + type(autoIcon))
    self.autoIcon =autoIcon

class Note:
  """The main class for notes. Contains variant content depending on notetype

  For the following NoteTypes the attribute content contains the following
  objects:

  COLOR          | single pybasket.Color object
  CROSSREFERENCE | single pybasket.CrossRef object
  FILE           | path (string) to an existing file to be included
  GROUP          | list of pybasket.Note objects
  HTML           | string of the html source of a Note
  IMAGE          | path (string) to an existing image file to be included
  LAUNCHER       | path (string) to an existing launcher file (.desktop)
  LINK           | single pybasket.LinkRef object
  SOUND          | path to an existing sound file (Likely obsolete)
  TEXT           | string of the source of a Note (Likely obsolete)

  Attributes:
      notetype: Specifies the type of objects found in the content argument
      content: Depending on notetype, this attributes holds different objects
      parent: Points back to the parent object; parent can be of type
        pybasket.Note or a pybasket.Basket.
      tagStates: Optional; List of pybasket.State. Each state must be a member
        of a pybasket.Tag object. Moreover, this Tag object must also be passed
        to the pybasket.BasketDirectory object to be stored properly
      width: Optional; width of the Note in the Basket Scene; in pixels. Is
        going to be ignored if parent is not a Basket with free layout
      x: Optional; horizontal coordinate of the Note in the Basket Scene; in
        pixel. Is ignored, if the parent is of type NoteType.GROUP, or the
        Basket has a column layout.
      y: Optional; vertical coordinate of the Note in the Basket Scene; in
        pixel. Is ignored, if the parent is of type NoteType.GROUP, or the
        Basket has a column layout.
      lastModification: Optional; Timestamp used for marking last modfication
      added: Optional; Timestamp which marks when the Note was added to a Basket
      folded: Optional; Only applies to NoteType.GROUP, is ignored otherwise
  """

  def __init__(self, notetype: NoteType, content, parent, tagStates = None, width: int = 0, x:int = 0, y:int = 0,
      lastModification:datetime = datetime.now(), added: datetime = datetime.now(), folded:bool = False):
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
      if tagStates is None:
         tagStates = []
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

  def __str__(self):
    return "Note of type: " + str(self.notetype) + ", content: " + str(self.content)

  ## Fetch basket or try one level up
  def getBasket(self):
    """ Returns this Note's parent basket

    If this Note is not in the root level of the basket, the Note's parent 
    object is called to return its parent Basket, recursively.

    Returns:
        the basket in which this note is contained
    """

    if not isinstance(self.parent,Basket):
      return self.parent.getBasket()
    else:
      return self.parent

  ## returns a flat structure of self and notes
  def flatNoteFamily(self):
    """ Gets list of this note and all children, recursively

    Returns:
        An unbranched/flat list of the calling Note and its children Notes.
    """

    flatlist = [self]

    if self.notetype == NoteType.GROUP:
      for child in self.content:
        flatlist.extend(child.flatNoteFamily())

    return flatlist


  ## to name the note source files sequentially, this function checks the outputpath for "note[0-9]*.html" files
  ## It returns the next highest number
  def getNextNoteName(self, outputpath: str) -> str:
    """Returns the next sequential name for a Note source file

    The function checks all files in the output path of this regex format:
    note-[0-9]+.html
    To the hightest number found 1 is added. This means intermittent gaps are
    ignored.

    Args:
        outputpath: Path where parent basket is saved

    Returns:
        Sequential, unique note filename within the outputpath of the format:
          outputpath/note-%d.html
    """

    highNum = 0

    note_regex = re.compile(r"note-([0-9]+).html")

    for f in os.listdir(outputpath):
      match = note_regex.match(f)
      if match:
        try:
          index = int(match.group(1))
          if index > highNum:
            highNum = index
        except ValueError:
          "The file does not contain an integer, skipping: " + f

    return os.path.join(outputpath, "note-%s.html" % str(highNum + 1))

  def writeXmlElement(self, doc, parent, basketpath: str):
    """Adds the xml tags representing this Note to the doc object

    This function which is called when the .basket xml file located in
    basketpath is created. All file paths, such as icons, and embedded files are
    truncated to keep the basename only. It is assumed the file will be located
    in basketpath. The respective files are moved by the
    pybasket.BasketDirectory into the basketpath. This might not be ideal...

    TODO: let this function move its content to basketpath

    Args:
        doc: the document object model(DOM) root
        parent: a pybasket.Basket, or a pybasket.Note which contains this Note
        basketpath: the final save path for the Basket containing this Note
    """

    if self.notetype == NoteType.GROUP:
      node = doc.createElement('group')

      node.setAttribute('folded', 'true' if self.folded else 'false')

      ## column group does not have any attributes 
      if not isinstance(self.parent,Basket):
        if(self.getBasket().disposition != Disposition.FREE):
          for note in self.content:
            note.writeXmlElement(doc, node, basketpath)
      else:

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

        ## TODO create file and copy in correct folder, check Note.writeXmlElement as well
        content = doc.createElement('content')

        filename = self.getNextNoteName(basketpath)

        with open(filename, "w") as f:
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
        content.appendChild(doc.createTextNode(str(self.content.url)))
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
  """Container class for properties of a pybasket.Basket object

  Attributes:
      bgColor: Optional; specifies the background color of the parent basket
      bgImage: Optional; path (string) to a valid image file, ideally .png
      textColor: Optional; specifies the default text color of the parent basket
  """
  def __init__(self, bgColor: Color = Color(), bgImage:str = "", textColor: Color = Color()):

    if not isinstance(bgColor, Color):
      raise TypeError('bgColor  must be of type Color')
    self.bgColor = bgColor

    self.bgImage = bgImage

    if not isinstance(textColor, Color):
      raise TypeError('textColor  must be of type Color')
    self.textColor = textColor


## Basket layout
class Disposition(Enum):
  """Enumerates layout property of a parent basket"""

  FREE = "free"
  COL1 = "col1"
  COL2 = "col2"
  COL3 = "col3"


class Shortcut:
  """A class containing a string specifying a Qt shortcut

  Attributes:
      combination: Optional; string for key sequence, such as "Ctrl+C"
  """

  def __init__(self, combination = ""):
    if not isinstance(combination, str):
      raise TypeError('must be a string: ' + combination)
    self.combination = combination


class Basket:
  """Main class to contstruct a basket page

  For column layouts, the notes argument must have a specific format.
  Each column is represented by a NoteType.GROUP Note which must encapsulate
  all other notes in the respective column. This means, for COL1 the notes
  argument must be a list of a single Note object (of type GROUP), for COL2 only
  two Notes of type GROUP must be in the notes argument, and so forth.

  Attributes:
      name: string which is printed as the basket's name
      notes: Optional; list of notes within this Basket object
      suggestedFoldername: Optional; a string which can be used as basename for
        this Basket's output path
      children: Optional; list of sub-level Baskets
      icon: Optional; icon which should be used for this Basket
      appearance: Optional; sets the colors for background and color, and can
        set the background image
      disposition: Optional; sets the general layout of the Basket.
        Default: free layout
      shortcut: Optional; key sequence to be registerted to open this Basket
      folded: Optional; if True, the tree of children Baskets is collapsed
  """
  def __init__(self, name, notes = None, suggestedFoldername="", children = None, icon = "knotes", 
      appearance = Appearance(), disposition = Disposition.FREE, shortcut = Shortcut(), folded = False):

    if not isinstance(name,str):
      raise TypeError('basket name is not of type string')
    self.name = name

    if not isinstance(suggestedFoldername,str):
      raise TypeError('basket suggestedFoldername is not of type string')
    self.suggestedFoldername = suggestedFoldername

    if children is None:
      children = []
    for basket in children:
      if not isinstance(basket, Basket):
        raise TypeError('There is a child basket which is not of type Basket')
    self.children = children


    if notes is None:
      notes = []
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
    output =  "Basket " + self.name 

    if len(self.children) > 0:
      clist = []
      for child in self.children:
        clist.append(str(child))

      return str(output + ", " + str(clist))
    else:
      return str(output)


  # returns self and all children in a list
  def flatBasketFamily(self):
    """returns a flat list of this basket and its children

    Returns:
        flattend list of this basket, followed by its children, recursively
    """
    flatlist = [self]

    if self.children != None:
      for child in self.children:
        if isinstance(child, Basket):
          flatlist.extend(child.flatBasketFamily())

    return flatlist

  def basketFamily(self):
    """returns a tree of baskets with this Basket at its root

    Returns:
       list of this object, followed by a list of its children, recursively
         e.g. [ self, [child1, child2] ]
    """
    thislist = []
    for child in self.children:
      thislist.append(child.basketFamily())
    return [ self, thislist ]

  def flatNoteFamily(self):
    """get all notes in this basket as flat list
    Returns:
        flattend list of notes in this Basket object
    """
    flatlist = []

    for note in self.notes:
      flatlist.extend(note.flatNoteFamily())

    return flatlist

  def writeBasket(self, basketpath:str ):
    """Main function to write .basket file for this Basket object in basketpath

    Is only called by the BasketDirectory object

    Args:
        basketpath: path in which this Basket should be saved
    """

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
    if self.appearance.bgColor != "":
      appearance.setAttribute('backgroundColor', str(self.appearance.bgColor))
    appearance.setAttribute('backgroundImage', self.appearance.bgImage)
    if self.appearance.textColor != "":
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
    """Adds entry to baskets.xml (DOCTYPE baskettree)

    Only called by pybasket.BasketTree

    Args:
        doc: the document object model(DOM) 
        parent: root level pybasket.BasketTree or another pybasket.Basket
    """
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
  """Class containing the relationships of baskets (tree)

  Attributes:
      baskets: a list of pybasket.Basket objects
  """
  def __init__(self, baskets):
    for basket in baskets:
      if not isinstance(basket, Basket):
        raise TypeError('There is an item in a BasketTree which is not of type Basket: ' + str(basket))
    self.baskets = baskets

  ## writes the baskets.xml into path
  def writeTree(self, basketDirectory):
    """Main function to write baskets.xml in the pybasket.BasketDirectory root

    Only called by the pybasket.BasketDirectory object which contains this object

    Args:
        basketDirectory: a path to the BasketDirectory object root
    """
    if not isinstance(basketDirectory, BasketDirectory):
      raise TypeError('the argument basketDirectory is not of type BasketDirectory')

    ## test whether all basket have valid suggestedFoldernames, set unique if necessary
    foldernamesList = []
    for basket in self.flatBasketFamily():
      if basket.suggestedFoldername == "" or basket.suggestedFoldername in foldernamesList:
        basket.suggestedFoldername = 'basket-' + uuid.uuid4().hex
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
    """returns a flat list of this basket and its children

    Returns:
        flattend list of this basket, followed by its children, recursively
          e.g. [ self, child1, child of child1, child2 ]
    """

    flatlist = []

    if self.baskets != None:
      for basket in self.baskets:
        if isinstance(basket, Basket):
          flatlist.extend(basket.flatBasketFamily())
    return flatlist

  def basketFamily(self):
    """returns a tree of baskets with this Basket at its root

    Returns:
       list of this object, followed by a list of its children, recursively
         e.g. [ self, [child1, child2] ]
    """

    thislist = []
    for basket in self.baskets:
      thislist.extend(basket.basketFamily())
    return [ self, thislist ]


## Tag's text style
class TextStyle:
  """Container class for font styling in a pybasket.Tag

  TODO: color does not know of a default color 

  Attributes:
      strikeOut: Optional; if True, the text is struck out (<del>)
      bold: Optional; if True, the text is set bold
      underline; Optional; if True, the text is set to be underlined
      color: Optional; Sets the text color
      italic: Optional; if True, the text is set italic
  """

  def __init__(self, strikeOut:bool = False, bold:bool = False, underline:bool = False, color: Color = Color(), italic:bool = False):
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
  """A class specifying the font for a pybasket.Tag

  Attributes:
      name: Optional; name of the font
      size: Optional; font size in points. If set to -1, the application uses
        the default size
  """

  def __init__(self, name: str = "", size: int = -1):
    if not isinstance(name, str):
      raise TypeError('The Font name argument must be a string')
    self.name = name

    ## not sure whether this must be the case of wether a double is valid as well
    if not isinstance(size, int):
      raise TypeError('The Font\'s size argument must be of type integer')
    self.size = size

## Tag states
class State:
  """Specifies an individual state of a pybasket.Tag

  A State is a property of a Tag. It can be used to construct a Tag with
  multiple individual states, such as a checkable todo-checkbox.
  The text and color properties modify the text styling of a Note referencing
  this State

  Attributes:
      name: string which is shown to the user as the label
      stateid: internally used string to reference this state, should only contain
        these characters [A-z0-9_]
      emblem: Optional; a name or path to an icon for this state
      textstyle: Optional; formatting for the text
      font: Optional; font family
      bgColor: Optional; background color for the referencing note
      textEquivalent: Optional; string to be used if there is no icon
        available, or if a Note with this State is copied, the text equivalent
        is inserted instead of an icon
      onAllTextLines: Optional; If True, Basket intends to apply the state to
        all lines of that note. But seems obsolete.
  """

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
    """Adds this State to the pybasket.Tag xml object specified by parent

    Only called by pybasket.Tag

    Args:
        doc: the document object model(DOM)
        parent: root level pybasket.BasketTagsTree
    """

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
    if self.textstyle.color != "":
      textstyle.setAttribute('color', str(self.textstyle.color))
    textstyle.setAttribute('italic', 'true' if self.textstyle.italic else 'false')
    node.appendChild(textstyle)

    font = doc.createElement('font')
    font.setAttribute('size', str(self.font.size))
    font.setAttribute('name', str(self.font.name))
    node.appendChild(font)


    if self.bgColor != "":
      bgcolor = doc.createElement('backgroundColor')
      bgcolor.appendChild(doc.createTextNode(str(self.bgColor)))
      node.appendChild(bgcolor)

    textequiv = doc.createElement('textEquivalent')
    textequiv.setAttribute('string', str(self.textEquivalent))
    textequiv.setAttribute('onAllTextLines', 'true' if self.onAllTextLines else 'false')
    node.appendChild(textequiv)

    parent.appendChild(node)


class Tag:
  """Class specifying tags to be applied on pybasket.Note objects

  Attributes:
      name: The string presented to the user as the Tag's name
      states: a list of pybasket.States. Must at least contain a single State
      shortcut: Optional; specifies shortcut to apply this Tag to a Note
      inherited: Optional; If true, creating new states for this Tag in the
        application will conviniently inherit the properties of this Tag
  """

  def __init__(self, name: str, states, shortcut: Shortcut = Shortcut(), inherited: bool = True):
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
    """Adds this Tag to the document object model specified by the argument doc

    Only called by pybasket.BasketTagsTree

    Args:
        doc: the document object model(DOM) 
        parent: root level pybasket.BasketTagsTree
    """

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
  """Class which contains the list of tags in a pybasket.BasketDirectory

  Only called by pybasket.BasketDirectory

  Attributes:
      tags: list of pybasket Tags
  """

  def __init__(self, tags):
    for tag in tags:
      if not isinstance(tag, Tag):
        raise TypeError('There is an item in BasketTagsTree which is not of type Tag')
    self.tags = tags

  # write tags.xml into path, and write emblems in respective path
  def writeTree(self, basketDirectory: str):
    """Main function to write the tags.xml file

    Only called by pybasket.BasketDirectory

    In the orignal source, the first Tag received the addtional attribute
    nextStateUid, which is kept here. Though it is hardcoded, and usage is not
    clear to the author at the point of writing this.

    Args:
        basketDirectory: a path (string) to the root of parent 
          pybasket.BasketDirectory
    """

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
  """Main class for this module

  In order to create a BasKet directory source, this object must be intantiated.
  After setting all the content of the BasKet source, the write() method must be
  called to output the source.

  Attributes:
      savepath: a path (string) into which the BasKet source is put out. Must
        not exist
      baskets: Optional; list of pybasket.Basket objects
      tags: Optional; list of pybasket.Tag objects to be added to the source
      additionalBackgrounds: Optional; list of file paths (str) for additionally
        included background images as convenience for the user of the compiled
        directory source
      additionalEmblems: Optional; list of file paths (str) to icons which 
        should be appended to the directory source
      additionalIcons: Optional; list of file paths (str) to icon which should
        be appended to the directory source
  """

  def __init__(self, savepath: str, baskets = None, tags = None, additionalBackgrounds = None, additionalEmblems = None, additionalIcons = None):
    if not isinstance(savepath, str):
      raise TypeError('The savepath argument must be a string')
    self.savepath = os.path.realpath(savepath)

    if baskets is None:
      baskets = []
    for basket in baskets:
      if not isinstance(basket, Basket):
        raise TypeError('the baskets argument must only contain objects of type Basket')
    self.baskets = baskets

    if tags is None:
      tags = []
    for tag in tags:
      if not isinstance(tag, Tag):
        raise TypeError('The tags argument must only contain objects of type Tag')
    self.tags = tags

    if additionalBackgrounds is None:
      additionalBackgrounds = []
    for bg in additionalBackgrounds:
      if not isinstance(bg, str) and not os.path.isfile(bg):
        raise FileNotFoundError('Every entry in additionalBackgrounds must be a valid file')
    self.additionalBackgrounds = additionalBackgrounds

    if additionalEmblems is None:
      additionalEmblems = []
    for emblem in additionalEmblems:
      if not isinstance(emblem, str) and not os.path.isfile(emblem):
        raise FileNotFoundError('Every entry in additionalEmblems must be a valid file')
    self.additionalEmblems = additionalEmblems

    if additionalIcons is None:
      additionalIcons = []
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
    """outputs this BasketDirectory to self.savepath"""

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
      if basket.appearance.bgImage != "":
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
