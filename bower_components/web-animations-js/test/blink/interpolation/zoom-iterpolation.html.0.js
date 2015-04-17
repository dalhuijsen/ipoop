
assertInterpolation({
  property: 'zoom',
  from: '1',
  to: '2'
}, [
  // Interpolated values are greater than 0, however this test case fails because
  // we clamp to 0.01.
  // {at: -5, is: '0.000000000000000000000000000000000000000000001'}, // zoom must be > 0
  {at: -0.3, is: '0.7'},
  {at: 0, is: '1'},
  {at: 0.3, is: '1.3'},
  {at: 0.6, is: '1.6'},
  {at: 1, is: '2'},
  {at: 1.5, is: '2.5'}
]);
