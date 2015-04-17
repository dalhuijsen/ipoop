
assertInterpolation({
  property: 'shape-image-threshold',
  from: '0.5',
  to: '1'
}, [
  {at: -1.5, is: '0'},
  {at: -0.5, is: '0.25'},
  {at: 0, is: '0.5'},
  {at: 0.5, is: '0.75'},
  {at: 1, is: '1'},
  {at: 1.5, is: '1'}
]);
