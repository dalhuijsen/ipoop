
assertInterpolation({
  property: 'width',
  from: '0px',
  to: '100px'
}, [
  {at: -0.3, is: '0px'}, // CSS width can't be negative.
  {at: 0, is: '0px'},
  {at: 0.3, is: '30px'},
  {at: 0.6, is: '60px'},
  {at: 1, is: '100px'},
  {at: 1.5, is: '150px'}
]);
assertInterpolation({
  property: 'width',
  from: '10px',
  to: '100%'
}, [
  {at: -0.3, is: '0px'}, // CSS width can't be negative.
  {at: 0, is: '10px'},
  {at: 0.3, is: '37px'},
  {at: 0.6, is: '64px'},
  {at: 1, is: '100px'},
  {at: 1.5, is: '145px'}
]);
