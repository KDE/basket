#!/usr/bin/env python3
# vim: ts=2 sw=2 et
#
# SPDX-FileCopyrightText: 2021 Sebastian Engel <kde@sebastianengel.eu>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
#

import os
import datetime
from pybasket import *


## Setting up i18n

import gettext

## Localization directory
localdir = './locale'

## Translation function
translate = gettext.translation('createWelcomeBaskets', localdir, fallback=True)
_ = translate.gettext


def main():

  ######################################################################################################################
  ## Define available states for
  ######################################################################################################################
  tagTitle = Tag(_("Title"),
      states = [
        State( name = _("Title"), stateid = "title", textstyle=TextStyle(bold=True),textEquivalent="##")
        ],
      shortcut = Shortcut(combination= "Ctrl+0"))

  tagTodo = Tag(_("To Do"),
      states = [
        State(name=_("Unchecked"), stateid="todo_unchecked", emblem="tag_checkbox", textEquivalent="[ ]"),
        State(name=_("Done"), stateid="todo_done", textstyle=TextStyle(strikeOut=True), 
          emblem="tag_checkbox_checked", textEquivalent="[x]"),
      ],
      shortcut = Shortcut(combination= "Ctrl+1"), inherited=True)

  tagProgess =  Tag(_("Progress"),
      states = [
        State(name=_("0 %"), stateid="progress_000", emblem="tag_progress_000", textEquivalent="[____]"),
        State(name=_("25 %"), stateid="progress_025", emblem="tag_progress_025", textEquivalent="[=___]"),
        State(name=_("50 %"), stateid="progress_050", emblem="tag_progress_050", textEquivalent="[==__]"),
        State(name=_("75 %"), stateid="progress_075", emblem="tag_progress_075", textEquivalent="[===_]"),
        State(name=_("100 %"), stateid="progress_100", emblem="tag_progress_100", textEquivalent="[====]"),
      ],
      shortcut = Shortcut(combination= "Ctrl+2"), inherited=True)

  tagHighlight  = Tag(_("Highlight"),
      states = [
        State(name = _("Highlight"), stateid = "highlight", textEquivalent="=>", bgColor=Color('#ffffcc'),
          textstyle = TextStyle( color = Color('#000000')))
        ],
      shortcut = Shortcut(combination= "Ctrl+5"))

  tagImportant = Tag(_("Important"),
      states = [
        State(name=_("Important"), stateid="important", emblem="tag_important", bgColor=Color('#ffcccc'), 
          textstyle = TextStyle( color = Color('#000000')), textEquivalent="!!")
        ],
      shortcut = Shortcut(combination= "Ctrl+6"))

  tagInfo = Tag(_("Information"),
      states = [
        State(name = "Information", stateid = "information", emblem="dialog-information", textEquivalent="(i)")
      ],
      shortcut = Shortcut(combination= "Ctrl+8"))

  tagIdea =  Tag(_("Idea"),
      states = [
        State(name=_("Idea"), stateid="idea", emblem = "ktip", textEquivalent="I.")
        ],
      shortcut = Shortcut(combination= "Ctrl+9"))
  
  tagPersonal =  Tag(_("Personal"), 
      states = [
        State(name=_("Personal"), stateid="personal", textstyle=TextStyle(color=Color('#008000')),textEquivalent="P.")
        ])

  tags = [ tagTitle, tagTodo, tagProgess, tagHighlight, tagImportant, tagInfo, tagIdea, tagPersonal ]


  ######################################################################################################################
  ## Setup Welcome basket
  ######################################################################################################################

  basketWelcome = Basket(name = _("Welcome"), suggestedFoldername = 'welcome', icon ='resources/basket-icons/folder_home',
      appearance = Appearance(bgImage = './resources/backgrounds/strings.png'), notes = [], children = [])

  ### Group

  noteW_G1 = Note( notetype=NoteType.GROUP, parent=basketWelcome, width=309, folded=False, x=209, y=63, content = [])

  noteW_G1_N1 = Note( notetype = NoteType.HTML, parent=noteW_G1, tagStates = [ tagTitle.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Welcome to BasKet Note Pads!
</body></html>
'''))
  noteW_G1.content.append(noteW_G1_N1)

  noteW_G1_N2 = Note( notetype = NoteType.HTML, parent=noteW_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Quickly write down ideas, collect various data and keep related things together.
</body></html>
'''))
  noteW_G1.content.append(noteW_G1_N2)

  basketWelcome.notes.append(noteW_G1)


  ###


  noteW_N3 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=391, x=19, y=172,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-style:italic">This multi-purpose note-taking application can helps you to:</span><br />- Easily take all sort of notes<br />- Collect research results and share them<br />- Centralize your project data and re-use them later<br />- Quickly organize your toughts in idea boxes<br />- Keep track of your information in a smart way<br />- Make intelligent To Do lists<br />- And a lot more...</p>
</body></html>
'''))
  basketWelcome.notes.append(noteW_N3)


  noteW_N4 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=257, x=455, y=208,
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:11pt;font-family:Bitstream Vera Sans">
<p>You can <a href="basket-internal-import">import data from your previous note-taking application</a> as a starting point.</p>
</body></html>
'''))
  basketWelcome.notes.append(noteW_N4)


  noteW_N5 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=360, x=19, y=417,
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>Once you finished to read the welcome baskets, you can <a href="basket-internal-remove-basket">delete them by clicking here</a>.<br />You will be able to view them later in the Help menu.</p>
</body></html>
'''))
  basketWelcome.notes.append(noteW_N5)


  noteW_N6 = Note( notetype = NoteType.IMAGE, parent = basketWelcome, width=163, x=20, y=33,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-11-20T14:17:24'),
      content = 'resources/files/note1.png')
  basketWelcome.notes.append(noteW_N6)


  noteW_N7 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=446, x=0, y=0,
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>To start using this application, click &quot;General&quot; in the left tree.</p>
</body></html>
'''))
  basketWelcome.notes.append(noteW_N7)


  noteW_N8 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=180, x=516, y=435,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>https://invent.kde.org/utilities/basket</p>
</body></html>)
'''))
  basketWelcome.notes.append(noteW_N8)



  noteW_N9 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=655, x=17, y=334,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-style:italic">Quick Introduction:</span><br />- Click anywhere to add a note and start typing<br />- <span style="font-weight:600">Drop</span> or <span style="font-weight:600">paste</span> anything into the baskets to add notes, or use the <span style="font-weight:600">Insert</span> menu<br />- You can create as many baskets as you want, to organize your information</p>
</body></html>
'''))
  basketWelcome.notes.append(noteW_N9)



  ######################################################################################################################
  ## Example Notes
  ######################################################################################################################

  basketExNotes= Basket(name = _("Example: Notes"), suggestedFoldername = 'notes', icon ='resources/basket-icons/knotes', 
      appearance = Appearance(bgColor = Color("#cce6ff"), bgImage = './resources/backgrounds/pins.png', textColor =Color("#000000")),
      notes = [], children = [])


  ### Group 1

  noteN_G1 = Note( notetype=NoteType.GROUP, parent=basketExNotes, width=334, folded=False, x=10, y=50, content = [])

  noteN_G1_N1 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>Various Notes:</p>
</body></html>
'''))
  noteN_G1.content.append(noteN_G1_N1)

  noteN_G1_N2 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-style:italic">Mike address:</span><br />245 East 47th Street, 44th Floor,<br />New York, NY 10017</p>
</body></html>
'''))
  noteN_G1.content.append(noteN_G1_N2)

  noteN_G1_N3 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagIdea.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>Is it possible to buy a new PC now?</p>
</body></html>
'''))
  noteN_G1.content.append(noteN_G1_N3)

  noteN_G1_N4 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>Evanescence concert: January, 13th, next year</p>
</body></html>
'''))
  noteN_G1.content.append(noteN_G1_N4)

  noteN_G1_N5 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T17:31:58'),
      content=_('''
<p><span style="font-style:italic">Heroes season 1</span>: watched up to episode 9<br /><span style="font-style:italic">Prison Break season 2</span>: watched up to episode 12</p>
</body></html>
'''))
  noteN_G1.content.append(noteN_G1_N5)

  noteN_G1_N6 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-style:italic">Next blog posts to do:</span><br />- My new dog<br />- Tim's recent acrobaties</p>
</body></html>
'''))
  noteN_G1.content.append(noteN_G1_N6)

  basketExNotes.notes.append(noteN_G1)

  ### Group 2


  noteN_G2 = Note( notetype=NoteType.GROUP, parent=basketExNotes, width=292, folded=False, x=367, y=47, content = [])

  noteN_G2_N7 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
To Do:
</body></html>
'''))
  noteN_G2.content.append(noteN_G2_N7)

  noteN_G2_N8 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTodo.states[1] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Email Joe to remind him Mike's address
</body></html>
'''))
  noteN_G2.content.append(noteN_G2_N8)

  noteN_G2_N9 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTodo.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Prepare dinner for friends
</body></html>
'''))
  noteN_G2.content.append(noteN_G2_N9)

  noteN_G2_N10 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagProgess.states[1] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>Project X5<br /><span style="font-style:italic;color:#808080">Prototype 452 finished</span></p>
</body></html>
'''))
  noteN_G2.content.append(noteN_G2_N10)

  basketExNotes.notes.append(noteN_G2)

  ### Group 3

  noteN_G3 = Note( notetype=NoteType.GROUP, parent=basketExNotes, width=263, folded=False, x=388, y=230, content = [])

  noteN_G3_N11 = Note( notetype=NoteType.HTML, parent=noteN_G3, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Links to visit:
</body></html>
'''))
  noteN_G3.content.append(noteN_G3_N11)
  
  noteN_G3_N12 = Note( notetype=NoteType.LINK, parent=noteN_G3, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content = LinkRef( title = "kde.org", url = "https://kde.org", icon="favicons/www.kde.org", autoTitle=True, autoIcon=True))
  noteN_G3.content.append(noteN_G3_N12)

  noteN_G3_N13 = Note( notetype=NoteType.LINK, parent=noteN_G3, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content = LinkRef( title = "google.com", url = "https://www.google.com", icon="favicons/www.google.com", autoTitle=True, autoIcon=True))
  noteN_G3.content.append(noteN_G3_N13)

  basketExNotes.notes.append(noteN_G3)


  ### Other Notes
  noteN_N14 = Note( notetype=NoteType.HTML, parent=basketExNotes, width=494, x=0 , y=0 ,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Here is a basket to show you some examples of usage for this application.<br>
This basket is used to take notes.
</body></html>
'''))
  basketExNotes.notes.append(noteN_N14)

  ######################################################################################################################
  ## Example Research
  ######################################################################################################################

  basketExRes = Basket(name = _("Example: Research"), suggestedFoldername = 'research', 
      icon ='resources/basket-icons/folder_violet',
      appearance = Appearance(bgColor = Color("#e6ccff"), 
        bgImage = './resources/backgrounds/pens.png', textColor =Color("#000000")))

  noteR_N1 = Note( notetype=NoteType.HTML, parent=basketExRes, width=515, x=0, y=0,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-29T00:11:19'),
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Here is a basket to show you some examples of usage for this application.<br>
This basket is used to keep track of research results for a project.
</body></html>
'''))
  basketExRes.notes.append(noteR_N1)


  noteR_N2 = Note( notetype=NoteType.HTML, parent=basketExRes, width=298, x=38, y=60,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:15:57'),
      tagStates = [ tagTitle.states[0], tagPersonal.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-size:14pt">Emperor Penguin</span></p>
</body></html>
'''))
  basketExRes.notes.append(noteR_N2)

  ### group 1 ##########################################################################################################

  noteR_G1 = Note( notetype=NoteType.GROUP, parent=basketExRes, folded=False, width=159, x=18, y=103, content = [])

  noteR_G1_N3 = Note( notetype=NoteType.IMAGE, parent=noteR_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:15:44'),
      content = "resources/files/Manchot_Empereur.jpg")
  noteR_G1.content.append(noteR_G1_N3)

  noteR_G1_N4 = Note( notetype=NoteType.LINK, parent=noteR_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:16:43'),
      content = LinkRef( title=_("From fr.Wikipedia.org"), 
        url="https://fr.wikipedia.org/wiki/Image:Manchot_Empereur.jpg", autoTitle=False, autoIcon=True))
  noteR_G1.content.append(noteR_G1_N4)

  basketExRes.notes.append(noteR_G1)


  ### other notes ######################################################################################################


  noteR_N5 = Note( notetype=NoteType.LINK, parent=basketExRes, width=196, x=346, y=146,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:19:10'),
      tagStates = [ tagTitle.states[0] ],
      content = LinkRef( title="Wikipedia article", 
        url="https://en.wikipedia.org/wiki/Emperor_Penguin", autoTitle=False, autoIcon=True))
  basketExRes.notes.append(noteR_N5)


  ### group 2 ##########################################################################################################


  noteR_G2 = Note( notetype=NoteType.GROUP, parent=basketExRes, folded=False, width=186, x=197, y=146, content = [])

  noteR_G2_N6 = Note( notetype=NoteType.HTML, parent=noteR_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:21:58'),
      tagStates = [ tagTitle.states[0] ],
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
People
</body></html>
'''))
  noteR_G2.content.append(noteR_G2_N6)

  noteR_G2_N7 = Note( notetype=NoteType.LINK, parent=noteR_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:22:06'),
      content = LinkRef( title="fred@internet.org", 
        url="mailto:fred@internet.org", autoTitle=True, autoIcon=True))
  noteR_G2.content.append(noteR_G2_N7)

  noteR_G2_N8 = Note( notetype=NoteType.LINK, parent=noteR_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:22:06'),
      content = LinkRef( title="steves@internet.org", 
        url="mailto:steves@internet.org", autoTitle=False, autoIcon=True))
  noteR_G2.content.append(noteR_G2_N8)

  noteR_G2_N9 = Note( notetype=NoteType.LINK, parent=noteR_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:22:06'),
      content = LinkRef( title="john@internet.org", 
        url="mailto:john@internet.org", autoTitle=False, autoIcon=True))
  noteR_G2.content.append(noteR_G2_N9)

  noteR_G2_N10 = Note( notetype=NoteType.HTML, parent=noteR_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:23:50'),
      content=_('''
<html><head><meta name="qrichtext" content="1" /></head><body>
IM: marie@jabber.org
</body></html>
'''))
  noteR_G2.content.append(noteR_G2_N10)

  basketExRes.notes.append(noteR_G2)

  ### group 3 ##########################################################################################################

  noteR_G3 = Note( notetype=NoteType.GROUP, parent=basketExRes, folded=False, width=251, x=412, y=164, content = [])

  noteR_G3_N11 = Note( notetype=NoteType.HTML, parent=noteR_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:24:37'),
      tagStates = [ tagTitle.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
To Do:
</body></html>
'''))
  noteR_G3.content.append(noteR_G3_N11)


  noteR_G3_N13 = Note( notetype=NoteType.HTML, parent=noteR_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:24:42'),
      tagStates = [ tagTodo.states[1] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
The table of content
</body></html>
'''))
  noteR_G3.content.append(noteR_G3_N13)

  noteR_G3_N14 = Note( notetype=NoteType.HTML, parent=noteR_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:24:50'),
      tagStates = [ tagTodo.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Everybody do a chapter
</body></html>
'''))
  noteR_G3.content.append(noteR_G3_N14)

  noteR_G3_N15 = Note( notetype=NoteType.HTML, parent=noteR_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:25:39'),
      tagStates = [ tagTodo.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
We merge everything
</body></html>
'''))
  noteR_G3.content.append(noteR_G3_N15)

  noteR_G3_N16 = Note( notetype=NoteType.HTML, parent=noteR_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:25:55'),
      tagStates = [ tagTodo.states[0], tagImportant.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Spellcheck and other checks
</body></html>
'''))
  noteR_G3.content.append(noteR_G3_N16)

  noteR_G3_N17 = Note( notetype=NoteType.HTML, parent=noteR_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:26:22'),
      tagStates = [ tagProgess.states[1] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
The graphics
</body></html>
'''))
  noteR_G3.content.append(noteR_G3_N17)

  basketExRes.notes.append(noteR_G3)

  ### group 4 ##########################################################################################################

  noteR_G4 = Note( notetype=NoteType.GROUP, parent=basketExRes, folded=False, width=203, x=13, y=350, content=[] )


  noteR_G4_N18 = Note( notetype=NoteType.HTML, parent=noteR_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:18:12'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-weight:600">Table of Content:</span><br />1. Physical characteristics<br />2. Ecology and behaviour<br />3. Reproduction<br />4. Conservation status<br />5. Miscellaneous<br />6. Further reading<br />7. External links</p>
</body></html>
'''))
  noteR_G4.content.append(noteR_G4_N18)

  noteR_G4_N19 = Note( notetype=NoteType.HTML, parent=noteR_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:26:51'),
      tagStates = [ tagHighlight.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
John: you STOLEN the Wikipedia table of content! We will not use the same
</body></html>
'''))
  noteR_G4.content.append(noteR_G4_N19)

  basketExRes.notes.append(noteR_G4)

  ### group 5 ##########################################################################################################

  noteR_G5 = Note( notetype=NoteType.GROUP, parent=basketExRes, folded=False, width=401, x=252, y=308, content = [] )


  noteR_G5_N20 = Note( notetype=NoteType.HTML, parent=noteR_G5,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:27:55'),
      tagStates = [ tagIdea.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>I propose those colors for the titles in the document:</p>
</body></html>
'''))
  noteR_G5.content.append(noteR_G5_N20)

  noteR_G5_N21 = Note( notetype=NoteType.COLOR, parent=noteR_G5,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:28:11'),
      content = Color("#002c6f"))
  noteR_G5.content.append(noteR_G5_N21)

  noteR_G5_N22 = Note( notetype=NoteType.COLOR, parent=noteR_G5,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:28:28'),
      content = Color("#e67800"))
  noteR_G5.content.append(noteR_G5_N22)

  basketExRes.notes.append(noteR_G5)


  ### other notes ######################################################################################################


  noteR_N22 = Note( notetype=NoteType.LAUNCHER, parent=basketExRes, width=225, x=257, y=385,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:30:05'),
      content = "resources/files/writer.desktop")
  basketExRes.notes.append(noteR_N22)

  noteR_N23 = Note( notetype=NoteType.FILE, parent=basketExRes, width=269, x=254, y=460,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:38:23'),
      content = "resources/files/Emperor Penguin.odt")
  basketExRes.notes.append(noteR_N23)


  ### group 6 ##########################################################################################################


  noteR_G6 = Note( notetype=NoteType.GROUP, parent=basketExRes, folded=True, width=233, x=523, y=389, content = [] )


  noteR_G6_N24 = Note( notetype=NoteType.HTML, parent=noteR_G6,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:21:19'),
      tagStates = [ tagTitle.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
The introduction (by Fred):
</body></html>
'''))
  noteR_G6.content.append(noteR_G6_N24)

  noteR_G6_N25 = Note( notetype=NoteType.HTML, parent=noteR_G6,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-15T00:20:50'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
The Emperor Penguin (Aptenodytes forsteri) is the largest of all penguins. It is the only penguin that breeds during the winter in Antarctica. Emperor Penguins eat mainly crustaceans (such as krill) but also indulge in consuming small fish and squid. In the wild, Emperor Penguins typically live for 20 years, but some records indicate a maximum lifespan of around 40 years. (Note that the King Penguin is a different species, and the Royal Penguin is a subspecies of the Rockhopper Penguin.)
</body></html>
'''))
  noteR_G6.content.append(noteR_G6_N25)

  basketExRes.notes.append(noteR_G6)


  ######################################################################################################################
  ## Tips
  ######################################################################################################################

  basketTips = Basket(name = _("Tips"), icon ='resources/basket-icons/ktip', suggestedFoldername = 'tips',
      appearance = Appearance(bgColor = Color("#ffffcc"), bgImage = './resources/backgrounds/light.png', 
        textColor = Color("#000000")), 
      notes = [], children = [], disposition = Disposition.COL1)

  noteT_column = Note( notetype=NoteType.GROUP, parent=basketTips, content = [])

  ## Group 1 ###########################################################################################################

  noteT_G1 = Note( notetype=NoteType.GROUP, parent=noteT_column, folded=False, content = [])


  noteT_G1_N1 = Note( notetype=NoteType.HTML, parent=noteT_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-07-09T22:23:01'),
      tagStates = [ tagTitle.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p dir="ltr"><span style="color:#0000cc">How to Use BasKet Note Pads</span></p>
</body></html>
'''))
  noteT_G1.content.append(noteT_G1_N1)

  noteT_G1_N2 = Note( notetype=NoteType.HTML, parent=noteT_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T12:29:23'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-style:italic">The basic concepts of this application:</span><br />- <span style="font-weight:600">Basket</span>: a document where you can type notes. This is that big yellow rectangle you're watching. Create different baskets for every subjects, topics, projects you wish.<br />- <span style="font-weight:600">Basket tree</span>: the tree on the left represents all your baskets. Click on one of them to make it active.<br />- <span style="font-weight:600">Note</span>: Baskets can contain a lot of notes. Notes are every single rectangles here. They help separate every ideas you have about a subject. You can have text, images, link, file notes...<br />- <span style="font-weight:600">Group</span>: You can group related notes together. A little [-] will appear in front of the group, letting you to only show the first note of the group, to temporarily hide non-important notes.<br />- <span style="font-weight:600">Tag</span>: You can assign several tags to every notes. You can mark a note as Important, as To Do, and then check the note to mark it as Done... You can also create your own tags, to categorize notes. Tags can change the appearance of the note by setting colors and fonts.<br />- <span style="font-weight:600">Filter</span>: Using the filter bar on top of this window, you can immediatly show only the notes that match the text you type, or the notes that have the tag you selected.<br />- <span style="font-weight:600">System tray icon</span>: BasKet Note Pads is always running in the system tray if you close the main window. Your notes are always at hand, very quickly.</p>
</body></html>
'''))
  noteT_G1.content.append(noteT_G1_N2)

  noteT_G1_N2 = Note( notetype=NoteType.HTML, parent=noteT_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T13:06:33'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can assign a different icon and background color to your baskets.<br />This greatly helps you recognize them quickly in the basket tree as it grows.</p>
</body></html>
'''))
  noteT_G1.content.append(noteT_G1_N2)

  noteT_G1_N3 = Note( notetype=NoteType.HTML, parent=noteT_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T12:27:22'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>There are two basket layouts: columns, where notes are automatically arranged for your convenience, and free-form, where you can put notes everywhere, like you do with a piece of paper.</p>
</body></html>
'''))
  noteT_G1.content.append(noteT_G1_N3)

  noteT_G1_N4 = Note( notetype=NoteType.HTML, parent=noteT_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-08-26T15:16:15'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="font-style:italic">There are three different options to create a new basket:</span><br />- <span style="font-weight:600">New Basket</span>: create a blank one on the very bottom of your basket tree (unless you specify a parent basket in the window that pops up).<br />- <span style="font-weight:600">New Sub-Basket</span>: create a basket into the current basket. The sub-basket will get the same background and image color, icon, layout... so you do not have to configure that.<br />- <span style="font-weight:600">New Sibling Basket</span>: create a basket at the same depth in the basket tree as the current basket. Also inherit the properties of the current basket. Practical if you represent different pages with sub-baskets: you can add a &quot;page&quot; very quickly.</p>
</body></html>
'''))
  noteT_G1.content.append(noteT_G1_N4)

  noteT_G1_N5 = Note( notetype=NoteType.HTML, parent=noteT_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-14T02:03:57'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can Ctrl+click a note to select or unselect it. It is quicker than clicking its handle or drawing a selection rectangle on it.<br />By Ctrl+clicking several notes, you can select several separated notes.</p>
</body></html>
'''))
  noteT_G1.content.append(noteT_G1_N5)

  noteT_G1_N6 = Note( notetype=NoteType.HTML, parent=noteT_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-07-09T22:23:13'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>To select several notes at once (to be able to group or move them, etc), draw a rectangle with the mouse.<br />Click, and without releasing the mouse button, move the mouse to create a blue rectangle that includes all the notes you want to select.<br />You then can move them together or click the Group button in the toolbar to put them together.</p>
</body></html>
'''))
  noteT_G1.content.append(noteT_G1_N6)

  noteT_column.content.append(noteT_G1)

  ### Group 2 ##########################################################################################################

  noteT_G2 = Note( notetype=NoteType.GROUP, parent=noteT_column, folded=False, content = [] )


  noteT_G2_N1 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      tagStates = [ tagTitle.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="color:#0000cc">General</span></p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N1)

  noteT_G2_N2 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can tag your notes to get more organized and find your data quicker.<br />For that, move your mouse on a note, click the little arrow that appeared on the left and check the tags you want to assign to that note.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N2)

  noteT_G2_N3 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
You can make check-lists with this mechanism by assigning the "To Do" tag and click the little square that appear to check the note.<br>
When inserting a note just before or after a note that have this "To Do" tag, the new note will also have the checkbox.
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N3)

  noteT_G2_N4 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-12-08T19:36:51'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>The notes that have tags with no visual clue (no icon, no color, no font) or with overriden ones, present ellipsis (&quot;...&quot;) at the left of the emblems, so you can always quickly see what notes are tagged.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N4)

  noteT_G2_N5 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-12-01T16:00:50'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>To select several object that are not contiguous, Ctrl+click each one to add or remove them from your selection.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N5)

  noteT_G2_G1 = Note ( notetype=NoteType.GROUP, parent=noteT_G2, folded=True, content = [] )

  noteT_G2_G1_N1 = Note( notetype=NoteType.HTML, parent=noteT_G2_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can double-click a note handle or a group handle to copy the note(s) to the selection. Click the little [+] on the left of this note to learn what is the selection.</p>
</body></html>
'''))
  noteT_G2_G1.content.append(noteT_G2_G1_N1)

  noteT_G2_G1_N2 = Note( notetype=NoteType.HTML, parent=noteT_G2_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-12-01T16:00:14'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
When you select a text in any application, it is automatically copied to a special clipboard named "Selection".<br>
You paste the selection with the middle mouse button.<br>
Double-clicking the handle of an iteam or a group let you paste it in one click.
</body></html>
'''))
  noteT_G2_G1.content.append(noteT_G2_G1_N2)

  noteT_G2.content.append(noteT_G2_G1)


  noteT_G2_N6 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-13T14:39:08'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>The KDE session managment restore every applications when you restart your session or your computer (active by default).<br />You do not need to exit BasKet Note Pads before closing your session: it will be automatically restarted during your next log in.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N6)

  noteT_G2_N7 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-14T02:04:43'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can definitively hide the text formatting toolbar so that it do not waste space on the screen.<br />Keyboard shortcuts (Ctrl+B for Bold, etc) will continue to work to change the formatting.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N7)


  noteT_G2_N8 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-01-31T20:24:02'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>BasKet Note Pads easily lets you backup and restore your baskets. Go to the Basket menu and choose Backup &amp; Restore...<br />It is recommanded to backup often to protect your data against a computer failure.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N8)

  noteT_G2_N9 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T13:34:05'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>BasKet Note Pads data are all stored in a hidden folder, generally in ~/.kde/share/apps/basket (where ~ represents your Home folder).<br />You can change that folder to a more visible one like ~/Notes or ~/Baskets for easy manual backups by going to the Basket menu and choose Backup &amp; Restore.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N9)

  noteT_G2_N10 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T18:14:06'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can use your own images for basket icons and tag icons. Simply open the icon chooser window, click &quot;Browse...&quot; and select your image file (the PNG format is recommended in order to use transparency).</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N10)


  noteT_G2_N11 = Note( notetype=NoteType.HTML, parent=noteT_G2,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T18:13:00'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>The Select All action (Ctrl+A) is progressive. It starts by selecting all the notes in the group where the current note is. If you press Ctrl+A a second time, all the note of the parent group will be added to the selection, etc. Then all the column of the current note is selected. And finally, all the notes of the basket are selected.<br />The Home key is also progressive in the same manner: the focus will be moved to the first note of the group, and then the first note of the parent group, the first note of the current column, and finally, the first note of the basket.<br />Same for the End key.</p>
</body></html>
'''))
  noteT_G2.content.append(noteT_G2_N11)

  noteT_column.content.append(noteT_G2)

  ### Group 3 ##########################################################################################################
 
  noteT_G3 = Note( notetype=NoteType.GROUP, parent=noteT_column, folded=True, content = [])


  noteT_G3_N1 = Note( notetype=NoteType.HTML, parent=noteT_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      tagStates = [ tagTitle.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="color:#0000cc">Save Time</span></p>
</body></html>
'''))
  noteT_G3.content.append(noteT_G3_N1)

  noteT_G3_N2 = Note( notetype=NoteType.HTML, parent=noteT_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-12-17T14:20:45'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
You can assign or remove a tag or a tag state to several notes at once if there are several selected notes.
</body></html>
'''))
  noteT_G3.content.append(noteT_G3_N2)

  noteT_G3_N3 = Note( notetype=NoteType.HTML, parent=noteT_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
To select a note, drag a selection rectangle on it: it's quicker than clicking the note handle.
</body></html>
'''))
  noteT_G3.content.append(noteT_G3_N3)

  noteT_G3_N4 = Note( notetype=NoteType.HTML, parent=noteT_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
You can move selected notes by dragging them: no need to drag the tiny handle in this case.
</body></html>
'''))
  noteT_G3.content.append(noteT_G3_N4)

  noteT_G3_N5 = Note( notetype=NoteType.HTML, parent=noteT_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:10'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can Ctrl+click a note to select or unselect it. It is quicker than clicking its handle or drawing a selection rectangle on it.<br />By Ctrl+clicking several notes, you can select several separated notes.</p>
</body></html>
'''))
  noteT_G3.content.append(noteT_G3_N5)

  noteT_G3_N6 = Note( notetype=NoteType.HTML, parent=noteT_G3,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T13:00:10'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>You can Shift+click a note, and then Shift+click a second note: every notes between the first and the second notes (including them) will be selected.</p>
</body></html>
'''))
  noteT_G3.content.append(noteT_G3_N6)

  noteT_column.content.append(noteT_G3)

  ### Group 4 ##########################################################################################################

  noteT_G4 = Note( notetype=NoteType.GROUP, parent=noteT_column, folded=True, content = [] )

  noteT_G4_N1 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      tagStates = [ tagTitle.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="color:#0000cc">Keyboard Navigation</span></p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N1)

  noteT_G4_N2 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>To insert a new text note at cursor position, press Insert.<br />This also works when you are editing another note, so you're not interrupted.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N2)

  noteT_G4_N3 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T12:46:29'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>To insert a new note other than text, press Alt+I to popup the Insert menu.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N3)

  noteT_G4_N4 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-03-03T15:12:44'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
Press Alt+T to popup the Tags menu and assign/remove tags to the selected notes.
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N4)

  noteT_G4_N5 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
You can move notes by maintaining the Ctrl and Shift keys, and pressing the arrow keys. For instance, Ctrl+Shift+Down to move down a note.
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N5)

  noteT_G4_N7 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-01T19:00:03'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
To switch between baskets, use Alt+Up and Alt+Down.<br>
You can fold or expand basket trees with Alt+Left and Alt+Right, like you would do in any other tree, but with the Alt key!
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N7)

  noteT_G4_N8 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T13:01:23'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>When the cursor is on the first note of a group, you can press Left to fold the group, or Right to expand the group.<br />When the cursor is in a group, you can press Left to go to the first note of the group.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N8)

  noteT_G4_N9 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T13:05:13'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>Look at the properties of a basket: you can assign a keyboard shortcut to every baskets, so they are always at hand in a matter of tenths of a second!</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N9)

  noteT_G4_N10 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-01-12T17:54:47'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>This application provides global shortcuts. Those are an easy and very quick way to interact with your baskets. For instance, you can paste the clipboard by pressing Ctrl+Alt+Shift+V, or copy the selected text of any application to the current basket with Ctrl+Alt+Shift+S. The most important is Ctrl+Alt+Shift+W: it allow to raise or to hide the main window. Press it, work with your basket and press it again: you are then ready to continue your work without BasKet Note Pads to ennoy you. Even better: Ctrl+Alt+Shift+T allow to add a text note. An idea pops up in your mind? Press Ctrl+Alt+Shift+T, write your idea, and then Ctrl+Alt+Shift+W and you're done. As you can see, this application is quite unobtrusive.<br />You can configure the global shortcuts (change or remove them, and even discover lot of other ones) by poping up the Settings menu and choosing &quot;Configure Global Shortcuts&quot;.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N10)

  noteT_G4_N11 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T13:11:25'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>When editing a text note or a file name, you can press Escape to close the editor and go back in &quot;note browsing&quot; mode.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N11)

  noteT_G4_N12 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T13:12:25'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>When the focus is on the filter bar, you can:<br />- Press Enter to move the focus to the filtered basket (to browse throught the matching notes). You still can press Escape from the basket to reset the filter.<br />- Press Escape to reset the filter and go back to the basket when you're done.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N12)

  noteT_G4_N13 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T18:53:11'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>If the focus is in the basket and you are not editing a note, pressing Escape unselect everything.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N13)

  noteT_G4_N14 = Note( notetype=NoteType.HTML, parent=noteT_G4,
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T16:46:34'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>In the Tag Customization window, you can use Ctrl+Up and Ctrl+Down to edit the previous and next tag or state.<br />You can use Ctrl+Shift+Up and Ctrl+Shift+Down to move the current tag or state.<br />You also can press Suppr while the tag tree is selected to delete the current state or tag.<br />Finally, you can press F2 in the tag tree to move the focus to the tag or state name field.</p>
</body></html>
'''))
  noteT_G4.content.append(noteT_G4_N14)

  noteT_column.content.append(noteT_G4)

  ### Group 5 ##########################################################################################################

  noteT_G5 = Note( notetype=NoteType.GROUP, parent=noteT_column, folded=True, content = [] )

  noteT_G5_N1 = Note( notetype=NoteType.HTML, parent=noteT_G5,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-12-15T18:35:19'),
      tagStates = [ tagTitle.states[0] ],
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p><span style="color:#0000cc">Advanced Tips</span></p>
</body></html>
'''))
  noteT_G5.content.append(noteT_G5_N1)

  noteT_G5_N2 = Note( notetype=NoteType.HTML, parent=noteT_G5,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-12-15T18:35:39'),
      content = _('''
<html><head><meta name="qrichtext" content="1" /></head><body>
<p>If you do not use KDE session management and want to start BasKet Note Pads with the Autostart feature, you can use the command-line parameter --start-hidden in the command to run. This hides the main window in the system tray icon on startup.<br />Typically, you should run the command &quot;basket --start-hidden&quot;.</p>
</body></html>
'''))
  noteT_G5.content.append(noteT_G5_N2)

  noteT_column.content.append(noteT_G5)

  ### Finishing column #################################################################################################

  basketTips.notes.append(noteT_column)



  ######################################################################################################################
  ## Finalizing
  ######################################################################################################################
  basketWelcome.children.append(basketExNotes)
  basketWelcome.children.append(basketExRes)
  basketWelcome.children.append(basketTips)

  baskets = [basketWelcome]

  rootPath = "welcome_source"
  basketDirectory = BasketDirectory(rootPath, baskets, tags)

  basketDirectory.write()

if __name__ == "__main__":
  main()

