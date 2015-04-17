
assertInterpolation({
  property: 'opacity',
  from: '0',
  to: '1'
}, [
  {at: -0.3, is: '0'}, // CSS opacity is [0-1].
  {at: 0, is: '0'},
  {at: 0.3, is: '0.3'},
  {at: 0.6, is: '0.6'},
  {at: 1, is: '1'},
  {at: 1.5, is: '1'}
]);
