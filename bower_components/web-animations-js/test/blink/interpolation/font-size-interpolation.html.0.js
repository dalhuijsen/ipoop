
assertInterpolation({
  property: 'font-size',
  from: '4px',
  to: '14px'
}, [
  {at: -2, is: '0px'},  // CSS font-size can't be negative.
  {at: -0.3, is: '1px'},
  {at: 0, is: '4px'},
  {at: 0.3, is: '7px'},
  {at: 0.6, is: '10px'},
  {at: 1, is: '14px'},
  {at: 1.5, is: '19px'},
]);

// Web Animations 1 does not support inherit.
// assertInterpolation({
//   property: 'font-size',
//   from: 'inherit',
//   to: '20px'
// }, [
//   {at: -2, is: '0px'},  // CSS font-size can't be negative.
//   {at: -0.3, is: '7px'},
//   {at: 0, is: '10px'},
//   {at: 0.3, is: '13px'},
//   {at: 0.6, is: '16px'},
//   {at: 1, is: '20px'},
//   {at: 1.5, is: '25px'},
// ]);
