
assertInterpolation({
  property: 'orphans',
  from: '10',
  to: '1'
}, [
  {at: -0.5, is: '15'},
  {at: 0, is: '10'},
  {at: 0.3, is: '7'},
  {at: 0.7, is: '4'},
  // Only positive integers are valid
  {at: 1, is: '1'},
  {at: 1.5, is: '1'}
]);
