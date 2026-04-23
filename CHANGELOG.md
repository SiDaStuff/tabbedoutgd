# Changelog

## v1.0.2

- split audio muting into separate music and SFX settings
- added an optional Geode notification when the mod activates or deactivates
- removed the unused `geode.node-ids` dependency
- bumped project metadata to `v1.0.2`

## v1.0.1

- fixed tabbed out detection by checking the actual cocos window focus state
- moved focus checking off `drawScene` and onto cocos scheduler updates
- kept the unfocused fps limiter disabled when vsync is on
- kept audio muting on volume changes instead of pausing channels

## v1.0.0

- added a background FPS limiter with a default of `10`
- added a setting to mute audio while the game is unfocused
- added basic project docs
