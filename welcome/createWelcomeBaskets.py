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


def main():

  ######################################################################################################################
  ## Define available states for
  ######################################################################################################################
  tagTitle = Tag("Title", 
      states = [
        State( name = "Title", stateid = "title", textstyle=TextStyle(bold=True),textEquivalent="##")
        ],
      shortcut = Shortcut(combination= "Ctrl+0"))

  tagTodo = Tag("To Do !!!", 
      states = [
        State(name="Unchecked", stateid="todo_unchecked", emblem="tag_checkbox", textEquivalent="[ ]"),
        State(name="Done", stateid="todo_done", textstyle=TextStyle(strikeOut=True), 
          emblem="tag_checkbox_checked", textEquivalent="[x]"),
      ],
      shortcut = Shortcut(combination= "Ctrl+1"), inherited=True)

  tagProgess =  Tag("Progress", 
      states = [
        State(name="0 %", stateid="progress_000", emblem="tag_progress_000", textEquivalent="[____]"),
        State(name="25 %", stateid="progress_025", emblem="tag_progress_025", textEquivalent="[=___]"),
        State(name="50 %", stateid="progress_050", emblem="tag_progress_050", textEquivalent="[==__]"),
        State(name="75 %", stateid="progress_075", emblem="tag_progress_075", textEquivalent="[===_]"),
        State(name="100 %", stateid="progress_100", emblem="tag_progress_100", textEquivalent="[====]"),
      ],
      shortcut = Shortcut(combination= "Ctrl+2"), inherited=True)

  tagHighlight  = Tag("Highlight", 
      states = [
        State(name = "Highlight", stateid = "highlight", textEquivalent="=>", bgColor=Color('#ffffcc'))
        ],
      shortcut = Shortcut(combination= "Ctrl+5"))

  tagImportant = Tag("Important", 
      states = [
        State(name="Important", stateid="important", emblem="tag_important", bgColor=Color('#ffcccc'),textEquivalent="!!")
        ],
      shortcut = Shortcut(combination= "Ctrl+6"))

  tagInfo = Tag("Information", 
      states = [
        State(name = "Information", stateid = "information", emblem="dialog-information", textEquivalent="(i)")
      ],
      shortcut = Shortcut(combination= "Ctrl+8"))

  tagIdea =  Tag("Idea", 
      states = [
        State(name="Idea", stateid="idea", emblem = "ktip", textEquivalent="I.")
        ],
      shortcut = Shortcut(combination= "Ctrl+9"))
  
  tagPersonal =  Tag("Personal", 
      states = [
        State(name="Personal", stateid="personal", textstyle=TextStyle(color=Color('#008000')),textEquivalent="P.")
        ])

  tags = [ tagTitle, tagTodo, tagProgess, tagHighlight, tagImportant, tagInfo, tagIdea, tagPersonal ]


  ######################################################################################################################
  ## Setup Welcome basket
  ######################################################################################################################

  basketWelcome = Basket(name = "Welcome", suggestedFoldername = 'welcome', icon ='resources/basket-icons/folder_home',
      appearance = Appearance(bgImage = './resources/backgrounds/strings.png'), notes = [], children = [])

  ### Group

  noteW_G1 = Note( notetype=NoteType.GROUP, parent=basketWelcome, width=309, folded=False, x=209, y=63, content = [])

  noteW_G1_N1 = Note( notetype = NoteType.HTML, parent=noteW_G1, tagStates = [ tagTitle.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body>
Welcome to BasKet Note Pads!
</body></html>
''')
  noteW_G1.content.append(noteW_G1_N1)

  noteW_G1_N2 = Note( notetype = NoteType.HTML, parent=noteW_G1,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body>
Quickly write down ideas, collect various data and keep related things together.
</body></html>
''')
  noteW_G1.content.append(noteW_G1_N2)

  basketWelcome.notes.append(noteW_G1)


  ###


  noteW_N3 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=391, x=19, y=172,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p><span style="font-style:italic">This multi-purpose note-taking application can helps you to:</span><br />- Easily take all sort of notes<br />- Collect research results and share them<br />- Centralize your project data and re-use them later<br />- Quickly organize your toughts in idea boxes<br />- Keep track of your information in a smart way<br />- Make intelligent To Do lists<br />- And a lot more...</p>
</body></html>
''')
  basketWelcome.notes.append(noteW_N3)


  noteW_N4 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=257, x=455, y=208,
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:11pt;font-family:Bitstream Vera Sans">
<p>You can <a href="basket-internal-import">import data from your previous note-taking application</a> as a starting point.</p>
</body></html>
''')
  basketWelcome.notes.append(noteW_N4)


  noteW_N5 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=360, x=19, y=417,
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p>Once you finished to read the welcome baskets, you can <a href="basket-internal-remove-basket">delete them by clicking here</a>.<br />You will be able to view them later in the Help menu.</p>
</body></html>
''')
  basketWelcome.notes.append(noteW_N5)


  noteW_N6 = Note( notetype = NoteType.IMAGE, parent = basketWelcome, width=163, x=20, y=33,
      lastModification = datetime.now(), added=datetime.fromisoformat('2006-11-20T14:17:24'),
      content = 'resources/files/note1.png')
  basketWelcome.notes.append(noteW_N6)


  noteW_N7 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=446, x=0, y=0,
      tagStates = [ tagHighlight.states[0], tagInfo.states[0] ],
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p>To start using this application, click &quot;General&quot; in the left tree.</p>
</body></html>
''')
  basketWelcome.notes.append(noteW_N7)


  noteW_N8 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=180, x=516, y=435,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p>https://invent.kde.org/utilities/basket</p>
</body></html>
''')
  basketWelcome.notes.append(noteW_N8)



  noteW_N9 = Note( notetype=NoteType.HTML, parent=basketWelcome, width=655, x=17, y=334,
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-05T17:20:41'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p><span style="font-style:italic">Quick Introduction:</span><br />- Click anywhere to add a note and start typing<br />- <span style="font-weight:600">Drop</span> or <span style="font-weight:600">paste</span> anything into the baskets to add notes, or use the <span style="font-weight:600">Insert</span> menu<br />- You can create as many baskets as you want, to organize your information</p>
</body></html>
''')
  basketWelcome.notes.append(noteW_N9)



  ######################################################################################################################
  ## Example Notes
  ######################################################################################################################

  basketExNotes= Basket(name = "Ex.: Notes", suggestedFoldername = 'notes', icon ='resources/basket-icons/knotes', 
      appearance = Appearance(bgColor = Color("#cce6ff"), bgImage = './resources/backgrounds/pins.png', textColor =Color("#000000")),
      notes = [])


  ### Group 1

  noteN_G1 = Note( notetype=NoteType.GROUP, parent=basketExNotes, width=334, folded=False, x=10, y=50, content = [])

  noteN_G1_N1 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p>Various Notes:</p>
</body></html>
''')
  noteN_G1.content.append(noteN_G1_N1)

  noteN_G1_N2 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p><span style="font-style:italic">Mike address:</span><br />245 East 47th Street, 44th Floor,<br />New York, NY 10017</p>
</body></html>
''')
  noteN_G1.content.append(noteN_G1_N2)

  noteN_G1_N3 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagIdea.states[0] ],
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p>Is it possible to buy a new PC now?</p>
</body></html>
''')
  noteN_G1.content.append(noteN_G1_N3)

  noteN_G1_N4 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p>Evanescence concert: January, 13th, next year</p>
</body></html>
''')
  noteN_G1.content.append(noteN_G1_N4)

  noteN_G1_N5 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2007-01-13T17:31:58'),
      content='''
<p><span style="font-style:italic">Heroes season 1</span>: watched up to episode 9<br /><span style="font-style:italic">Prison Break season 2</span>: watched up to episode 12</p>
</body></html>
''')
  noteN_G1.content.append(noteN_G1_N5)

  noteN_G1_N6 = Note( notetype=NoteType.HTML, parent=noteN_G1, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p><span style="font-style:italic">Next blog posts to do:</span><br />- My new dog<br />- Tim's recent acrobaties</p>
</body></html>
''')
  noteN_G1.content.append(noteN_G1_N6)

  basketExNotes.notes.append(noteN_G1)

  ### Group 2


  noteN_G2 = Note( notetype=NoteType.GROUP, parent=basketExNotes, width=292, folded=False, x=367, y=47, content = [])

  noteN_G2_N7 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body>To Do:</body></html>
''')
  noteN_G2.content.append(noteN_G2_N7)

  noteN_G2_N8 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTodo.states[1] ],
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body>Email Joe to remind him Mike's address</body></html>
''')
  noteN_G2.content.append(noteN_G2_N8)

  noteN_G2_N9 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTodo.states[0] ],
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body>Prepare dinner for friends</body></html>
''')
  noteN_G2.content.append(noteN_G2_N9)

  noteN_G2_N10 = Note( notetype=NoteType.HTML, parent=noteN_G2, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagProgess.states[1] ],
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body style="font-size:8pt;font-family:DejaVu Sans">
<p>Project X5<br /><span style="font-style:italic;color:#808080">Prototype 452 finished</span></p>
</body></html>
''')
  noteN_G2.content.append(noteN_G2_N10)

  basketExNotes.notes.append(noteN_G2)

  ### Group 3

  noteN_G3 = Note( notetype=NoteType.GROUP, parent=basketExNotes, width=263, folded=False, x=388, y=230, content = [])

  noteN_G3_N11 = Note( notetype=NoteType.HTML, parent=noteN_G3, 
      lastModification = datetime.now(), added=datetime.fromisoformat('2005-11-04T13:24:09'),
      tagStates = [ tagTitle.states[0] ],
      content='''
  <html><head><meta name="qrichtext" content="1" /></head><body>Links to visit:</body></html>
  ''')
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
      content='''
<html><head><meta name="qrichtext" content="1" /></head><body>
Here is a basket to show you some examples of usage for this application.<br>
This basket is used to take notes.
</body></html>
''')
  basketExNotes.notes.append(noteN_N14)

  basketWelcome.children.append(basketExNotes)

  ######################################################################################################################
  ## Example Research
  ######################################################################################################################

  basketExRes = Basket(name = "Ex.: Research", suggestedFoldername = 'research', icon ='resources/basket-icons/folder_violet',
      appearance = Appearance(bgColor = Color("#e6ccff"), bgImage = './resources/backgrounds/pens.png', textColor =Color("#000000")))





  basketWelcome.children.append(basketExRes)

  ######################################################################################################################
  ## Tips
  ######################################################################################################################
  basketTips = Basket(name = "Tips", icon ='resources/basket-icons/ktip', suggestedFoldername = 'tips',
      appearance = Appearance(bgColor = Color("#ffffcc"), bgImage = './resources/backgrounds/light.png', textColor =Color("#000000")), disposition = Disposition.COL1)



  basketWelcome.children.append(basketTips)

  ######################################################################################################################
  ## Finalizing
  ######################################################################################################################

  baskets = [basketWelcome]

  rootPath = "welcome_source"
  basketDirectory = BasketDirectory(rootPath, baskets, tags)

  basketDirectory.write()

if __name__ == "__main__":
  main()

