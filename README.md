Tito
====

With small (but awesome) additions, this is still The GIMP.

See the [demo](http://youtu.be/G0PuH1LFWhA?hd=1) for an insight into the usage.


Tito is a text input tool developed for the GIMP as a first attempt 
to add an  
 `intent driven interface` that helps to get at the menus faster.

If,
 - You forgot where something appears in the menu,
 - or find it cumbersome to traverse through the menu everytime,
 - or forgot a shortcut,
 - or choose what you want from a bunch of smiliar actions,  
 then Tito should help you.

`Press [Shift+?] to activate Tito`

### See the following sections for more information:
 - [Algorithm](#i-algorithm)
 - [UI/UX](#ii-ui-ux)
 - [Ideas](#iii-ideas-not-yet-implemented)
 - [Pitfalls](#iv-pitfalls)  
  
  
## [I] Algorithm

### Data 
##### *What is searched?*
    1.User-readable texts of Menuitems, Tools, Dockable-dialogs and Plugins:
        Labels(as they appear in the menus)
        Tooltips(small descriptions) of all actions
    2. User history [Previously applied actions through Tito]
    3. Avoides search in main-menus, popups and context-actions

### Mechanism
      1.Labels first
      2.If keyword is two characters,
          then match them with first letters of first and second word in the labels.
          [Ex: 'gb' will list 'Gaussian Blur...']
      3.Tooltips
          When keyword is longer than two characters
          [Tooltips need to be searched because they might contain vital keywords
           that might be absent in the labels. Ex: 'adjust' will list 'Canvas Size...']
      4.User history
          If keyword matches with previously applied actions, it is listed first.
          [Also, the list of previously used actions appears when the down-arrow key is pressed]

### Adaptiveness
      Frequently used actions appear higher in the results list.
      This is great when dealing with frequently used menuitems that don't have shortcuts.
      Over a period of time, this would largely reduce the number of keystrokes.  
  
  
## [II] UI / UX

### Keyword entry
      Occupies minimal space, does not hinder with work-area
      Has the look and feel of a search-bar with editable text area
      Preferences:
          Position
              -Right top      [Default, and most used location for a search bar]
              -Middle         [For those who can't take their eyes off the canvas]
              -User specific  [Adjust by giving coordinates]
                              [The coordinates are in percentages that are limited by the current width]
          Width
              Adjustable from 20% to 60% of screen width_hbox
      
### Results list
      A navigable dropdown-list of runnable actions
      Each result shows:
        - The icon corresponding to the action
        - The Label as in the menus
        - A corresponding shortcut if available
        - A short description (tooltip) if available
      A result, after selection, can be 'applied' by 
        -Hitting return
        -Double click
      Preferences:
          Height  [Show more/less results]            

### Toggle actions
      Actions like 'Show Grid' or "Single-Window Mode" are displayed with
      appropriate icons that indicate their state

### Autohide
      A preference that would help toggle the visibility of Tito.
      If not set, then Tito would remain over GIMP
      and the user would press [Shift+?] to switch to Tito
      
### Show inert actions
      If set, then the actions that are currently unavailable
      would be shown, but 'greyed-out'. If not set, they would not be shown.

### Quick apply
      Hitting return while in the keyword area runs the first result.  

  
  
## [III] Ideas (not yet implemented)
      
### Thesaurus
      Map the dictionary of image editing with the GIMP terminology.
      So, 'scale', 'resize', 'shrink', 'expand' could transform to a single meaning.

### Wider search
      Search through the user help/documentation.
      Search based on menu hierarchy.

### Highlight keywords in result

### Command system
      An epic addition to the GIMP interface that would allow entering commands
      or a sequence of commands to perform complex manipulations.  
      
  
  
## [IV] Pitfalls

### Menu Path
      Tried and failed in attaining the menu-path, ui-path that corresponds to an action.
      There is apparently no Gtk mechanism to do this.
      The only way to do this would be to parse the entire menu ui XML,
      which would increase the complexity enormously and put limitations on performance.

### Autocomplete
      To reduce the number of keystrokes to search and apply, we tried different mechanisms
      of 'auto-complete' and settled with the very basics.

### Advanced search
      We tried using a search engine to parse through XML databases that contained 
      organized information. We indexed the text and used APIs to invoke the search engine.
      This turned out to be an overkill.