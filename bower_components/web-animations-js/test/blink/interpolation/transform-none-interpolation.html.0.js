

// Mismatched interpolation with an empty list should not use decomposition.
assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'none',
  to: 'rotate(180deg)'
}, [
  {at: -1, is: 'rotate(-180deg)'},
  {at: 0, is: 'none'},
  {at: 0.25, is: 'rotate(45deg)'},
  {at: 0.75, is: 'rotate(135deg)'},
  {at: 1, is: 'rotate(180deg)'},
  {at: 2, is: 'rotate(360deg)'},
]);
assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'rotate(180deg)',
  to: 'none'
}, [
  {at: -1, is: 'rotate(360deg)'},
  {at: 0, is: 'rotate(180deg)'},
  {at: 0.25, is: 'rotate(135deg)'},
  {at: 0.75, is: 'rotate(45deg)'},
  {at: 1, is: 'none'},
  {at: 2, is: 'rotate(-180deg)'},
]);
assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'none',
  to: 'rotate(360deg)'
}, [
  {at: -1, is: 'rotate(-360deg)'},
  {at: 0, is: 'none'},
  {at: 0.25, is: 'rotate(90deg)'},
  {at: 0.75, is: 'rotate(270deg)'},
  {at: 1, is: 'rotate(360deg)'},
  {at: 2, is: 'rotate(720deg)'},
]);
