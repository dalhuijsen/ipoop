

test(function() {
  assert_throws({name: 'NotSupportedError'}, function() {
    div.animate([{width: '100px'}], 1);
  }, 'A keyframe list with a single keyframe should cause an exception.');
},
'single keyframe tests',
{
  help:   'http://dev.w3.org/fxtf/web-animations/#the-unaccumulated-animation-value-of-a-keyframe-animation-effect',
  assert: 'Only keyframes with matched start and end properties are considered',
  author: 'Shane Stephens'
});

test(function() {
  assert_throws({name: 'NotSupportedError'}, function() {
    div.animate([{height: '100px'}, {width: '100px'}], 1);
  }, 'Mismatched start and end keyframes should cause an exception.');

  try {
    div.animate([{width: '100px'}, {height: '200px', offset: 0}, {width: '100px', offset: 1}, {height: '100px'}], 1);
  } catch (e) {
    assert_unreached("multiple start and end keyframes should be considered");
  }
},
'multiple keyframe tests',
{
  help:   'http://dev.w3.org/fxtf/web-animations/#the-unaccumulated-animation-value-of-a-keyframe-animation-effect',
  assert: 'Only keyframes with matched start and end properties are considered',
  author: 'Shane Stephens'
});

